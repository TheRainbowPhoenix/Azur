#include <num/num.h>
using namespace libnum;

extern "C" {

// num32_of_num8: %=2 && [extu.b] && [shll8]
num32 num32_of_num8(num8 x)
{
    return num32(x);
}

// num32_of_num16: %=2 && [exts.w] && [shll8]
num32 num32_of_num16(num16 x)
{
    return num32(x);
}

// num32_of_num64: %=[xtrct]
num32 num32_of_num64(num64 x)
{
    return num32(x);
}

// num32_mul: [dmuls.l] && [xtrct]
num32 num32_mul(num32 x, num32 y)
{
    return x * y;
}

// num32_dmul: [dmuls.l]
num64 num32_dmul(num32 x, num32 y)
{
    return num32::dmul(x, y);
}

// num32_eq: [bt* || bf*] && [shll16]
bool num32_eq(num32 x, int i)
{
    return x == i;
}

// num32_le: ![bt* || bf*]
bool num32_le(num32 x, int i)
{
    return x <= i;
}

// num32_gt: ![bt* || bf*]
bool num32_gt(num32 x, int i)
{
    return x > i;
}

// num32_le_0: %<=3
bool num32_le_0(num32 x)
{
    return x <= num32(0);
}

// num32_ge_0: %<=3
bool num32_ge_0(num32 x)
{
    return x >= num32(0);
}

// num32_gt_0: %<=3
bool num32_gt_0(num32 x)
{
    return x > num32(0);
}

// num32_lt_0: %<=3
bool num32_lt_0(num32 x)
{
    return x < num32(0);
}

} /* extern "C" */
