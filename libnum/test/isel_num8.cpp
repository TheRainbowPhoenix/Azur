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

}
