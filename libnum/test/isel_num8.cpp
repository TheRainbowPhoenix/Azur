#include <num/num.h>

using namespace num;

extern "C" {

// num8_of_num16: %=0
num8 num8_of_num16(num16 x)
{
    return num8(x);
}

// num8_of_num32: %=1 && [shlr8]
num8 num8_of_num32(num32 x)
{
    return num8(x);
}

// num8_of_num64: %<=2 && %=[shlr*||shad]
num8 num8_of_num64(num64 x)
{
    return num8(x);
}

/* This requires sign extensions because we care about high bits */
// num8_mul: %<=5
num8 num8_mul(num8 x, num8 y)
{
    return x * y;
}

// num8_eq: [or] && ![bt* || bf*]
bool num8_eq(num8 x, int i)
{
    return x == i;
}

// num8_le: ![bt* || bf*] && %<=4
bool num8_le(num8 x, int i)
{
    return x <= i;
}

// num8_gt: ![bt* || bf*] && %<=4
bool num8_gt(num8 x, int i)
{
    return x > i;
}

} /* extern "C" */
