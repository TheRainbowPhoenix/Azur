#include <num/num.h>
#include <stdio.h>
using namespace libnum;

/* Digits of the decimal part, from most to least significant. Returns the
   number of digits (which is 0 when x=0). The operand should be nonnegative.
   The string should have room for 32 characters; no NUL is added. */
template<typename T, typename I, int N> requires(is_num<T> && N >= 0)
static int decimalDigitsBase(char *str, T x)
{
    I v = x.frac().v;
    int i = 0;

    while(v != 0) {
        v *= 10;
        I digit = v >> N;
        str[i++] = (v >> N) + '0';
        v -= digit << N;
    }
    return i;
}

template<typename T> requires(is_num<T>)
static int decimalDigits(char *str, T x) {
    return 0;
}
template<> int decimalDigits<num8>(char *str, num8 x) {
    return decimalDigitsBase<num8, int, 8>(str, x);
}
template<> int decimalDigits<num16>(char *str, num16 x) {
    return decimalDigitsBase<num16, int, 8>(str, x);
}
template<> int decimalDigits<num32>(char *str, num32 x) {
    return decimalDigitsBase<num32, int, 16>(str, x);
}
template<> int decimalDigits<num64>(char *str, num64 x) {
    return decimalDigitsBase<num64, int64_t, 32>(str, x);
}

/* TODO: Complex string representations like %f/%e/%g
   Generates the string representation of x in str; returns the number of
   characters written. A NUL terminator is added (but not counted in the return
   value). A total of 45 bytes is required for the longest 64-bit value. */
template<typename T> requires(is_num<T>)
static int toString(char *str, T x)
{
    int n = 0;
    /* We need to be able to represent the opposite of INT_MIN */
    int64_t integral_part = (int)x;

    if(x.v == 0) {
        str[0] = '0';
        str[1] = 0;
        return 1;
    }
    if(x.v < 0) {
        *str = '-';
        n++;
        /* This might overflow, which is why we separated the integral part */
        x = -x;
        integral_part = -integral_part - (x.frac().v != 0);
    }

    n += sprintf(str + n, "%lld", integral_part);
    if(x.frac().v != 0) {
        str[n++] = '.';
        n += decimalDigits<T>(str + n, x);
    }
    str[n] = 0;
    return n;
}

int num8::strToBuffer(char *str)
{
    return toString<num8>(str, *this);
}
int num16::strToBuffer(char *str)
{
    return toString<num16>(str, *this);
}
int num32::strToBuffer(char *str)
{
    return toString<num32>(str, *this);
}
int num64::strToBuffer(char *str)
{
    return toString<num64>(str, *this);
}
