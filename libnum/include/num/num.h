//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// num.num: Fixed-point numerical types
//
// This header provides numerical types of various fixed-point sizes. The base
// type num is num32, and other data structures outside of this header
// (vectors, matrices, etc.) default to it. Other types are useful for storage
// and sometimes intermediate computation steps.
//---

/* TODO: Conversion with float/double: use the binary format efficiently
   General idea for a num -> fp conversion:
   1. Start with mantissa=num_value, exponent=num_fixed_position
   2. Decrease exponent and shift mantissa until top bit is 1, then shift again
   3. Generate the floating-point value
   General idea for an fp -> num conversion:
   1. Literally just shift mantissa by exponent - num_fixed_position */

/* TODO: Template specializations for std::integral_constant<int, VALUE> that
   inlines at compile time to either (1) true/false if out of bounds, or (2)
   coerce the int to the fixed point type */

#pragma once

#include <cstdint>
#include <cstddef>

#include <type_traits>
#include <concepts>

namespace num {

struct num8;
struct num16;
struct num32;
struct num64;

using num = num32;

/* num8: unsigned 0:8 fixed-point type
   * Size:        8 bits (1 byte)
   * Range:       0.0 (0x00) ... 0.996094 (0xff)
   * Precision:   0.0039 (1/256)
   * Represents:  <integer value> / 256

   This type is useful to store values of low precision in the 0..1 range. The
   value 1 cannot be represented, but it can sometimes be handled as a special
   case (interpolation curves) or emitted entirely (restricting the range). */
struct num8
{
    uint8_t v;

    inline constexpr num8(): v(0) {}
    /* Convert from int; pretty useless, but for completeness. */
    inline constexpr num8(int): v(0) {}
    /* Convert from float */
    inline constexpr num8(float f): v(f * 256) {}
    /* Convert from double */
    inline constexpr num8(double d): v(d * 256) {}
    /* Convert from other num types */
    inline constexpr explicit num8(num16 n);
    inline constexpr explicit num8(num32 n);
    inline constexpr explicit num8(num64 n);

    /* Convert to int; equally useless, but then again... */
    inline constexpr explicit operator int() { return 0; }
    /* Convert to float */
    inline constexpr explicit operator float() { return (float)v / 256; }
    /* Convert to double */
    inline constexpr explicit operator double() { return (double)v / 256; }

    /* Basic arithmetic */

    inline constexpr num8 &operator+=(num8 const &other) {
        v += other.v;
        return *this;
    }
    inline constexpr num8 &operator-=(num8 const &other) {
        v -= other.v;
        return *this;
    }
    inline constexpr num8 &operator*=(num8 const &other) {
        v = (v * other.v) >> 8;
        return *this;
    }
    inline constexpr num8 &operator/=(num8 const &other) {
        v = (v * 256) / other.v;
        return *this;
    }
    inline constexpr num8 &operator%=(num8 const &other) {
        v %= other.v;
        return *this;
    }
};

/* num16: Signed 8:8 fixed-point type
   * Size:        16 bits (2 bytes)
   * Range:       -128.0 (0x8000) ... 127.996094 (0x7fff)
   * Precision:   0.0039 (1/256)
   * Represents:  <integer value> / 256

   This type is useful to store numeric parameters that have a limited range.
   Using it in actual computations requires sign-extensions, but it is useful
   in multiplications because the 16-bit multiplication (muls.w) takes only 1
   cycle, and the num16 x num16 -> num32 result is immediately available. */
struct num16
{
    int16_t v;

    inline constexpr num16(): v(0) {}
    /* Convert from int */
    inline constexpr num16(int i): v(i * 256) {}
    /* Convert from float */
    inline constexpr num16(float f): v(f * 256) {}
    /* Convert from double */
    inline constexpr num16(double d): v(d * 256) {}
    /* Convert from other num types */
    inline constexpr explicit num16(num8 n);
    inline constexpr explicit num16(num32 n);
    inline constexpr explicit num16(num64 n);

    /* Convert to int */
    inline constexpr explicit operator int() { return v >> 8; }
    /* Convert to float */
    inline constexpr explicit operator float() { return (float)v / 256; }
    /* Convert to double */
    inline constexpr explicit operator double() { return (double)v / 256; }

    /* num16 x num16 -> num32 multiplication
       This is efficiently implemented with a muls.l instruction. */
    static constexpr num32 dmul(num16 const &x, num16 const &y);

    /* Basic arithmetic */

    inline constexpr num16 &operator+=(num16 const &other) {
        v += other.v;
        return *this;
    }
    inline constexpr num16 &operator-=(num16 const &other) {
        v -= other.v;
        return *this;
    }
    inline constexpr num16 &operator*=(num16 const &other) {
        v = (v * other.v) >> 8;
        return *this;
    }
    inline constexpr num16 &operator/=(num16 const &other) {
        v = (v * 256) / other.v;
        return *this;
    }
    inline constexpr num16 &operator%=(num16 const &other) {
        v %= other.v;
        return *this;
    }
};

/* num32: Signed 16:16 fixed-point type
   * Size:        32 bits (4 bytes)
   * Range:       -32768.0 (0x80000000) ... 32767.999985 (0x7fffffff)
   * Precision:   0.000015 (1/65536)
   * Represents:  <integer value> / 65536

   This is the ubiquitous fixed-point type in this library, most functions and
   types use it. It can be used pretty freely in ways similar to a float, with
   the important drawback that overflows are very possible. */
struct num32
{
    int32_t v;

    inline constexpr num32(): v(0) {}
    /* Convert from int */
    inline constexpr num32(int i): v(i * 65536) {}
    /* Convert from float */
    inline constexpr num32(float f): v(f * 65536) {}
    /* Convert from double */
    inline constexpr num32(double d): v(d * 65536) {}
    /* Convert from other num types */
    inline constexpr explicit num32(num8 n);
    inline constexpr explicit num32(num16 n);
    inline constexpr explicit num32(num64 n);

    /* Convert to int */
    inline constexpr explicit operator int() const {
        return v >> 16;
    }
    /* Convert to float */
    inline constexpr explicit operator float() const {
        return (float)v / 65536;
    }
    /* Convert to double */
    inline constexpr explicit operator double() const {
        return (double)v / 65536;
    }

    /* num32 x num32 -> num64 multiplication
       This is efficiently implemented with a dmuls.l instruction. */
    static constexpr num64 dmul(num32 const &x, num32 const &y);

    /* Basic arithmetic */

    inline constexpr num32 &operator+=(num32 const &other) {
        v += other.v;
        return *this;
    }
    inline constexpr num32 &operator-=(num32 const &other) {
        v -= other.v;
        return *this;
    }
    inline constexpr num32 &operator*=(num32 const &other) {
        v = ((int64_t)v * (int64_t)other.v) >> 16;
        return *this;
    }
    inline constexpr num32 &operator/=(num32 const &other) {
        v = ((int64_t)v * 65536) / other.v;
        return *this;
    }
    inline constexpr num32 &operator%=(num32 const &other) {
        v %= other.v;
        return *this;
    }
};

/* Arithmetic with integers */

inline constexpr num32 operator*(int n, num32 x) {
    num32 r;
    r.v = n * x.v;
    return r;
}
inline constexpr num32 operator*(num32 x, int n) {
    num32 r;
    r.v = n * x.v;
    return r;
}
inline constexpr num32 operator/(num32 x, int n) {
    num32 r;
    r.v = x.v / n;
    return r;
}

/* num64: Signed 32:32 fixed-point type
   * Size:        64 bits (8 bytes)
   * Range:       -2147483648.0 ... 2147483647.999999998
   * Precision:   2.33e-10 (1/4294967296)
   * Represents:  <integer value> / 4294967296

   This fixed-point type with extra precision can be used for intermediate
   computations when num32 would overflow. */
struct num64
{
    int64_t v;

    inline constexpr num64(): v(0) {}
    /* Convert from int */
    inline constexpr num64(int i): v((int64_t)i * 4294967296) {}
    /* Convert from float */
    inline constexpr num64(float f): v(f * 4294967296) {}
    /* Convert from double */
    inline constexpr num64(double d): v(d * 4294967296) {}
    /* Convert from other num types */
    inline constexpr explicit num64(num8 n);
    inline constexpr explicit num64(num16 n);
    inline constexpr explicit num64(num32 n);

    /* Convert to int */
    inline constexpr explicit operator int() { return v >> 32; }
    /* Convert to float */
    inline constexpr explicit operator float() { return (float)v/4294967296; }
    /* Convert to double */
    inline constexpr explicit operator double() {return (double)v/4294967296;}

    /* Basic arithmetic */

    inline constexpr num64 &operator+=(num64 const &other) {
        v += other.v;
        return *this;
    }
    inline constexpr num64 &operator-=(num64 const &other) {
        v -= other.v;
        return *this;
    }
    /* TOOD: Multiplication and division of mul64
    inline constexpr num64 &operator*=(num64 const &other) {
        v = ...;
        return *this;
    }
    inline constexpr num64 &operator/=(num64 const &other) {
        v = ...;
        return *this;
    } */
    inline constexpr num64 &operator%=(num64 const &other) {
        v %= other.v;
        return *this;
    }
};

/* The following concept identifies the four num types */
template<typename T>
concept is_num =
    std::same_as<T, num8> ||
    std::same_as<T, num16> ||
    std::same_as<T, num32> ||
    std::same_as<T, num64>;

/* Converting constructors */

inline constexpr num8::num8(num16 n): v(n.v) {}
/* Casting to unsigned allows the use of shlr instead of shad */
inline constexpr num8::num8(num32 n): v((uint32_t)n.v >> 8) {}
/* Slightly inefficient; acceses both longwords of n.v, only one is needed */
inline constexpr num8::num8(num64 n): v(n.v >> 24) {}

inline constexpr num16::num16(num8 n): v(n.v) {}
/* Casting to unsigned allows the use of shlr instead of shad */
inline constexpr num16::num16(num32 n): v((uint32_t)n.v >> 8) {}
inline constexpr num16::num16(num64 n): v(n.v >> 24) {}

inline constexpr num32::num32(num8 n): v(n.v * 256) {}
inline constexpr num32::num32(num16 n): v(n.v * 256) {}
inline constexpr num32::num32(num64 n): v(n.v >> 16) {}

inline constexpr num64::num64(num8 n): v((uint64_t)n.v * 16777216) {}
/* Pretty slow (~10 cycles) because of sign-extension across registers */
inline constexpr num64::num64(num16 n): v((int64_t)n.v * 16777216) {}
inline constexpr num64::num64(num32 n): v((int64_t)n.v * 65536) {}

/* Internal comparisons */

template<typename T> requires(is_num<T>)
inline constexpr bool operator==(T const &left, T const &right) {
    return left.v == right.v;
}

template<typename T> requires(is_num<T>)
inline constexpr bool operator!=(T const &left, T const &right) {
    return left.v != right.v;
}
template<typename T> requires(is_num<T>)
inline constexpr bool operator<(T const &left, T const &right) {
    return left.v < right.v;
}
template<typename T> requires(is_num<T>)
inline constexpr bool operator<=(T const &left, T const &right) {
    return left.v <= right.v;
}
template<typename T> requires(is_num<T>)
inline constexpr bool operator>(T const &left, T const &right) {
    return left.v > right.v;
}
template<typename T> requires(is_num<T>)
inline constexpr bool operator>=(T const &left, T const &right) {
    return left.v >= right.v;
}

/* Internal arithmetic operators */

template<typename T> requires(is_num<T>)
inline constexpr T operator+(T left, T const &right) {
    return (left += right);
}

template<typename T> requires(is_num<T>)
inline constexpr T operator-(T left, T const &right) {
    return (left -= right);
}

template<typename T> requires(is_num<T>)
inline constexpr T operator*(T left, T const &right) {
    return (left *= right);
}

template<typename T> requires(is_num<T>)
inline constexpr T operator/(T left, T const &right) {
    return (left /= right);
}

template<typename T> requires(is_num<T>)
inline constexpr T operator%(T left, T const &right) {
    return (left %= right);
}

template<typename T> requires(is_num<T>)
inline constexpr T operator+(T const &op) {
    return op;
}

template<typename T> requires(is_num<T>)
inline constexpr T operator-(T const &op) {
    return T(0) - op;
}

/* Other specific operations */

inline constexpr num32 num16::dmul(num16 const &x, num16 const &y)
{
    num32 n;
    n.v = x.v * y.v;
    return n;
}

inline constexpr num64 num32::dmul(num32 const &x, num32 const &y)
{
    num64 n;
    n.v = (int64_t)x.v * (int64_t)y.v;
    return n;
}

} /* namespace libnum */
