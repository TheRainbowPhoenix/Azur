#include <num/num.h>
using namespace libnum;

/* Integer square root (rather slow) */
static int64_t sqrtll(int64_t n)
{
    if(n < 4)
        return (n > 0);

    int64_t low_bound = sqrtll(n / 4) * 2;
    int64_t high_bound = low_bound + 1;

    return (high_bound * high_bound <= n) ? high_bound : low_bound;
}

num32 num32::sqrt() const
{
    num32 r;
    r.v = sqrtll((int64_t)v << 16);
    return r;
}
