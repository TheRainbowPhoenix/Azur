//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// unit_check.h: Utilities for asserting and printing
//
// This header defines a ToString<T> typeclass and the utility functions
// runWithChecker() which extend the provided function with a Checker argument.
// This object accumulates the results of tests, and stores the names and
// values of the test subjects so they can be printed if any assertion fails.
//---

#include <string>
#include <array>
#include <tuple>
#include <stdio.h>
#include <num/num.h>

template<typename T>
struct ToString {};

template<typename T>
concept to_string = requires { ToString<T>::str; };

template<typename T> requires(is_num<T>)
struct ToString<T>
{
    static std::string str(T x) {
        char s[64];
        uint64_t v = (uint64_t)x.v;
        if constexpr (sizeof x < 8)
            v &= ((1ull << 8 * sizeof x) - 1);
        sprintf(s, "%0*lx (%lf)", 2 * (int)sizeof(x), v, (double)x);
        return s;
    }
};

template<>
struct ToString<int>
{
    static std::string str(int i) {
        return std::to_string(i);
    }
};

template<typename... Ts> requires(to_string<Ts> && ...)
class Checker
{
public:
    Checker(): m_success(true) {}

    void vars(std::initializer_list<char const *> names) {
        m_names = std::vector(names);
    }
    void values(Ts... values) {
        m_values = std::make_tuple(values...);
    }

    bool check(bool b, char const *expr) {
        if(!b) {
            m_success = false;
            fprintf(stderr, "FAILED: %s\n", expr);
            printValues(std::index_sequence_for<Ts...> {});
        }
        return b;
    }

    bool successful() const {
        return m_success;
    }

private:
    /* Iterate on `m_names` and `m_values` following the index sequence Is; for
       each index, prints the variable name and value. */
    template<std::size_t... Is>
    void printValues(std::index_sequence<Is...>) {
        (fprintf(stderr, "  %s: %s\n", m_names[Is],
            ToString<typename std::tuple_element<Is, decltype(m_values)>::type>
              ::str(std::get<Is>(m_values)).c_str()), ...);
    }

    std::vector<char const *> m_names;
    std::tuple<Ts...> m_values;
    bool m_success;
};

template<typename T> requires(to_string<T>)
bool runWithChecker(std::function<void(T, Checker<T> &)> f)
{
    Checker<T> c;
    runOnSampleInputs<T>([f, &c](T x) { f(x, c); });
    return c.successful();
}

template<typename T, typename U> requires(to_string<T> && to_string<U>)
bool runWithChecker(std::function<void(T, U, Checker<T, U> &)> f)
{
    Checker<T, U> c;
    runOnSampleInputs<T, U>([f, &c](T x, U y) { f(x, y, c); });
    return c.successful();
}
