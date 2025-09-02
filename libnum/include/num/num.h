//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// num.num: Fixed-point numerical types
//
// This header provides fixed-point numerical types with the following shapes:
//   num8:    0: 8, unsigned  (only values 0.x)
//   num16:   8: 8, signed    (from -128 to 127.x)
//   num32:  16:16, signed    (from -32768 to 32767.x)
//   num64:  32:32, signed    (from -2147483648 to 2147483647.x)
//
// The canonical type for computations is `num`, which is an alias for num32.
//
// Type safety is enforced by never allowing implicit casts whenever that would
// narrow the range. To convert a num64 to a num32 or an int to a num16, the
// constructor must be called explicitly. This means that overflows can only
// happen during computation and explicit conversion, which are easier to track
// down and check than implicit conversions.
//
// TODO: Currently all constructors are explicit, even eg. num8 -> num16
//
// The API for each fixed-point type consists of:
// - Explicit constructors and cast operators to convert to and from integers,
//   floating-point types and other fixed-point types.
// - Minimum and maximum values as int (minInt, maxInt) and double (minDouble,
//   maxDouble). Note that num64 values don't fit in a double so the accuracy
//   of the latter two isn't perfect.
// - Basic arithmetic (+, -, *, /, %, +=, -=, *=, /=, %=).
// - Comparisons with itself and with int (range-safe).
// - Conversions to strings:
//   * strToBuffer(): appends the string representation of the value and a NUL
//     byte to a char *; returns the number of characters (excluding the NUL).
//     (TODO: experimental)
//   * TODO: Other string conversion functions with more options
// - floor(), ceil() and frac() methods
// - TODO: More functions to do the equivalent of <math.h> without floats
//---

/* TODO: Conversion with float/double: use the binary format efficiently
   General idea for a num -> fp conversion:
   1. Start with mantissa=num_value, exponent=num_fixed_position
   2. Decrease exponent and shift mantissa until top bit is 1, then shift again
   3. Generate the floating-point value
   General idea for an fp -> num conversion:
   1. Literally just shift mantissa by exponent - num_fixed_position */

#pragma once

#include <num/primitives.h>
#include <cstddef>
#include <type_traits>
#include <concepts>

namespace libnum {

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
    inline constexpr num8(unsigned int): v(0) {}
    /* Convert from float/double */
    inline constexpr num8(float f): v(f * 256) {}
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
    inline constexpr num8 ifloor() const {
        return 0;
    }
    inline constexpr num8 floor() const {
        return num8(0);
    }
    inline constexpr num8 iceil() const {
        return v != 0;
    }
    /* Warning: num8::ceil() always overflows! */
    inline constexpr num8 ceil() const {
        return num8(0);
    }
    inline constexpr num8 frac() const {
        num8 x;
        x.v = v;
        return x;
    }
    inline constexpr num8 abs() const {
        return *this;
    }

    /* Comparisons with int */

    inline constexpr bool operator==(int const &i) const {
        return (v | i) == 0;
    }
    inline constexpr bool operator<(int const &i) const {
        return i >= 1;
    }
    inline constexpr bool operator>=(int const &i) const {
        return i <= 0;
    }
    inline constexpr bool operator<=(int const &i) const {
        return i + !v > 0;
    }
    inline constexpr bool operator>(int const &i) const {
        return i + !v <= 0;
    }

    /* Limits as int */
    static constexpr int minInt = 0;
    static constexpr int maxInt = 0;
    /* Limits as double */
    static constexpr double minDouble = 0.0;
    static constexpr double maxDouble = double(0xff) / 256;

    /* String representations */
    int strToBuffer(char *str);
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
    inline constexpr num16(unsigned int u): v(u * 256) {}
    /* Convert from float/double */
    inline constexpr num16(float f): v(f * 256) {}
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
       This is efficiently implemented with a muls.w instruction. */
    static constexpr num32 dmul(num16 const &x, num16 const &y);
    /* num16 / num16 -> num16 division for positive numbers
       This bypasses some sign tests, which saves a bit of time. */
    static constexpr num16 div_positive(num16 const &x, num16 const &y);

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
        v = (v * other.v) / 256;
        return *this;
    }
    inline num16 &operator/=(num16 const &other) {
        v = prim::div_i32_i16_i16(v * 256, other.v);
        return *this;
    }
    inline constexpr num16 &operator%=(num16 const &other) {
        v %= other.v;
        return *this;
    }
    inline constexpr int ifloor() const {
        return v >> 8;
    }
    inline constexpr num16 floor() const {
        num16 x;
        x.v = v & 0xff00;
        return x;
    }
    inline constexpr int iceil() const {
        return (v + 0xff) >> 8;
    }
    inline constexpr num16 ceil() const {
        num16 x;
        x.v = ((v - 1) | 0xff) + 1;
        return x;
    }
    inline constexpr num16 frac() const {
        num16 x;
        x.v = v & 0xff;
        return x;
    }
    inline constexpr num16 abs() const {
        num16 x;
        x.v = (v < 0) ? -v : v;
        return x;
    }

    /* Comparisons with int */

    inline constexpr bool operator==(int const &i) const {
        return (int16_t)i == i && (i << 8) == v;
    }
    inline constexpr bool operator<(int const &i) const {
        return (v >> 8) < i;
    }
    inline constexpr bool operator>=(int const &i) const {
        return (v >> 8) >= i;
    }
    /* Unfortunately the branchless version for this test is expressed in terms
       of `v`, not `i`, so it does not simplify well when `i` is known. In that
       case, writing eg. `x > num16(0)` is faster than `x > 0`. */
    inline constexpr bool operator<=(int const &i) const {
        return (v >> 8) + ((v & 0xff) != 0) <= i;
    }
    inline constexpr bool operator>(int const &i) const {
        return (v >> 8) + ((v & 0xff) != 0) > i;
    }

    /* Limits as int */
    static constexpr int minInt = 0;
    static constexpr int maxInt = 0x7f;
    /* Limits as double */
    static constexpr double minDouble = -128.0;
    static constexpr double maxDouble = double(0x7fff) / 256;

    /* String representations */
    int strToBuffer(char *str);
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
    inline constexpr num32(unsigned int u): v(u * 65536) {}
    /* Convert from float/double */
    inline constexpr num32(float f): v(f * 65536) {}
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
    inline constexpr int ifloor() const {
        return v >> 16;
    }
    inline constexpr num32 floor() const {
        num32 x;
        x.v = v & 0xffff0000;
        return x;
    }
    inline constexpr int iceil() const {
        return (v + 0xffff) >> 16;
    }
    inline constexpr num32 ceil() const {
        num32 x;
        x.v = ((v - 1) | 0xffff) + 1;
        return x;
    }
    inline constexpr num32 frac() const {
        num32 x;
        x.v = v & 0xffff;
        return x;
    }
    inline constexpr num32 abs() const {
        num32 x;
        x.v = (v < 0) ? -v : v;
        return x;
    }
    num32 sqrt() const;

    /* Comparisons with int */

    inline constexpr bool operator==(int const &i) const {
        return (int16_t)i == i && (i << 16) == v;
    }
    inline constexpr bool operator<(int const &i) const {
        return (v >> 16) < i;
    }
    inline constexpr bool operator>=(int const &i) const {
        return (v >> 16) >= i;
    }
    inline constexpr bool operator<=(int const &i) const {
        return (v >> 16) + ((v & 0xffff) != 0) <= i;
    }
    inline constexpr bool operator>(int const &i) const {
        return (v >> 16) + ((v & 0xffff) != 0) > i;
    }

    /* Limits as int */
    static constexpr int minInt = 0;
    static constexpr int maxInt = 0x7fff;
    /* Limits as double */
    static constexpr double minDouble = -32768.0;
    static constexpr double maxDouble = double(0x7fffffff) / 65536;

    /* String representations */
    int strToBuffer(char *str);
};

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
    inline constexpr num64(unsigned int u): v((int64_t)u * 65536) {}
    /* Convert from float/double */
    inline constexpr num64(float f): v(f * 4294967296) {}
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
    inline constexpr int ifloor() const {
        return v >> 32;
    }
    inline constexpr num64 floor() const {
        num64 x;
        x.v = v & 0xffffffff00000000ull;
        return x;
    }
    inline constexpr int iceil() const {
        return (v >> 32) + ((uint32_t)v != 0);
    }
    inline constexpr num64 ceil() const {
        num64 x;
        x.v = ((v - 1) | 0xffffffffull) + 1;
        return x;
    }
    inline constexpr num64 frac() const {
        num64 x;
        x.v = v & 0xffffffffull;
        return x;
    }
    inline constexpr num64 abs() const {
        num64 x;
        x.v = (v < 0) ? -v : v;
        return x;
    }

    /* Limits as int */
    static constexpr int minInt = 0;
    static constexpr int maxInt = 0x7fffffff;
    /* Limits as double; note that the double doesn't have enough precision to
       represent the entirety of the maximum value. */
    static constexpr double minDouble = -2147483648.0;
    static constexpr double maxDouble = 2147483648.0 - double(1) / 2147483648;

    /* String representations */
    int strToBuffer(char *str);
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
/* Casting to 32-bit eliminates the unused high word */
inline constexpr num8::num8(num64 n): v((uint32_t)n.v >> 24) {}

inline constexpr num16::num16(num8 n): v(n.v) {}
/* Casting to unsigned allows the use of shlr instead of shad */
inline constexpr num16::num16(num32 n): v((uint32_t)n.v >> 8) {}
inline constexpr num16::num16(num64 n): v(n.v >> 24) {}

inline constexpr num32::num32(num8 n): v(n.v * 256) {}
inline constexpr num32::num32(num16 n): v((int32_t)n.v * 256) {}
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

/* Internal minima, maxima and clamp */

template<typename T> requires(is_num<T>)
inline constexpr T min(T const &left, T const &right)
{
    return (left < right) ? left : right;
}

template<typename T> requires(is_num<T>)
inline constexpr T max(T const &left, T const &right)
{
    return (left > right) ? left : right;
}

template<typename T> requires(is_num<T>)
inline constexpr T clamp(T const &val, T const &lower, T const &upper)
{
    return max(lower, min(val, upper));
}

/* Arithmetic with integers */

template<typename T> requires(is_num<T>)
inline constexpr T operator*(int n, T const &x) {
    T r;
    r.v = n * x.v;
    return r;
}

template<typename T> requires(is_num<T>)
inline constexpr T operator*(T const &x, int n) {
    T r;
    r.v = n * x.v;
    return r;
}

template<typename T> requires(is_num<T>)
inline constexpr T operator/(T const &x, int n) {
    T r;
    r.v = x.v / n;
    return r;
}

/* Comparison with double */

template<typename T> requires(is_num<T>)
inline constexpr bool operator<(T const &x, double const &y) {
    return x < T(y);
}

template<typename T> requires(is_num<T>)
inline constexpr bool operator>=(T const &x, double const &y) {
    return x >= T(y);
}

template<typename T> requires(is_num<T>)
inline constexpr bool operator<=(T const &x, double const &y) {
    return x <= T(y);
}

template<typename T> requires(is_num<T>)
inline constexpr bool operator>(T const &x, double const &y) {
    return x > T(y);
}

/* Other specific operations */

inline constexpr num32 num16::dmul(num16 const &x, num16 const &y)
{
    num32 n;
    n.v = x.v * y.v;
    return n;
}

inline constexpr num16 num16::div_positive(num16 const &x, num16 const &y)
{
    num16 r;
    r.v = ((uint32_t)(uint16_t)x.v << 8) / (uint16_t)y.v;
    return r;
}

inline constexpr num64 num32::dmul(num32 const &x, num32 const &y)
{
    num64 n;
    n.v = (int64_t)x.v * (int64_t)y.v;
    return n;
}

/* Floor modulo. We provide an optimized version for constants, which optimizes
   away the main condition. */
template<typename T> requires(is_num<T>)
inline constexpr T modf(T const &x, T const &y) {
    T r = x % y;
    return (r.v && (r.v ^ y.v) < 0) ? r + y : r;
}

/* Fractional part between (0, 1]; like frac(), but returns 1 for integers. */
template<typename T> requires(is_num<T>)
inline constexpr T fracCeil(T x) {
    T y;
    y.v = x.v - 1;
    y = y.frac();
    y.v = y.v + 1;
    return y;
}

/* Floor but round integers down */
template<typename T> requires(is_num<T>)
inline constexpr T previousInteger(T x) {
    T y;
    y.v = x.v - 1;
    return y.floor();
}

} /* namespace libnum */
