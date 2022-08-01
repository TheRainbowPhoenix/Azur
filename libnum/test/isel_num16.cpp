#include <num/num.h>
using namespace libnum;

extern "C" {

// num16_of_num8: %=1 && [extu.b]
num16 num16_of_num8(num8 x)
{
    return num16(x);
}

// num16_of_num32: %=1 && [shlr8]
num16 num16_of_num32(num32 x)
{
    return num16(x);
}

// num16_of_num64: %=[sh* || or]
num16 num16_of_num64(num64 x)
{
    return num16(x);
}

// num16_mul: [shad] && ![jsr]
num16 num16_mul(num16 x, num16 y)
{
    return x * y;
}

// num16_dmul: [muls.w]
num32 num16_dmul(num16 x, num16 y)
{
    return num16::dmul(x, y);
}

// num16_eq: [bt* || bf*] && [shll8]
bool num16_eq(num16 x, int i)
{
    return x == i;
}

// num16_le: ![bt* || bf*]
bool num16_le(num16 x, int i)
{
    return x <= i;
}

// num16_gt: ![bt* || bf*]
bool num16_gt(num16 x, int i)
{
    return x > i;
}

// num16_le_0: %<=3
bool num16_le_0(num16 x)
{
    return x <= num16(0);
}

// num16_ge_0: %<=3 || (%=4 && [mov.l])
bool num16_ge_0(num16 x)
{
    return x >= num16(0);
}

// num16_gt_0: %<=3
bool num16_gt_0(num16 x)
{
    return x > num16(0);
}

// num16_lt_0: %<=3 || (%=4 && [mov.l])
bool num16_lt_0(num16 x)
{
    return x < num16(0);
}

} /* extern "C" */
