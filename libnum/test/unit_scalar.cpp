//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// unit_scalar.cpp: Unit tests for scalar arithmetic

#include <stdio.h>
#include "unit_sample.h"
#include "unit_check.h"

/* Automatically stringify expressions in arguments to Checker.check() */
#define CHECK(EXPR) check(EXPR, #EXPR)

/* Test equality of num values up to slight variations */
template<typename T> requires(is_num<T>)
bool isEqUpTo(T x, T y, int i)
{
    return abs(x.v - y.v) <= i;
}

template<typename T>
bool runInternalArithmeticTest(int error)
{
    return runWithChecker<T, T>([error](T x, T y, Checker<T, T> &c) {
        c.vars({ "x", "y" });
        c.values(x, y);

        /* Surprisinly the cast back to integer in T() does saturation
           arithmetic, so we can't require equality in case of overflow. */

        double sum = double(x) + double(y);
        if(sum >= T::minDouble && sum <= T::maxDouble)
            c.CHECK(x + y == T(sum));

        double diff = double(x) - double(y);
        if(diff >= T::minDouble && diff <= T::maxDouble)
            c.CHECK(x - y == T(diff));

        double prod = double(x) * double(y);
        if(prod >= T::minDouble && prod <= T::maxDouble)
            c.CHECK(isEqUpTo(x * y, T(prod), error));

        if(y != 0) {
            double quot = double(x) / double(y);
            if(quot >= T::minDouble && quot <= T::maxDouble)
                c.CHECK(x / y == T(quot));
        }

        c.CHECK(y <= 0 || x < 0 || x % y < y);
        c.CHECK((x < y) + (x == y) + (x > y) == 1);
        c.CHECK((x <= y) - (x == y) + (x >= y) == 1);
   });
}

template<typename T>
bool runIntegerComparisonTest()
{
    return runWithChecker<T, int>([](T x, int i, Checker<T, int> &c) {
        c.vars({ "x", "i" });
        c.values(x, i);
        c.CHECK((x < i) + (x == i) + (x > i) == 1);
        c.CHECK((x <= i) - (x == i) + (x >= i) == 1);
    });
}

int main(void)
{
    bool success = true;

    printf("Testing unary laws on num8...\n");
    success &= runWithChecker<num8>([](num8 x, Checker<num8> &c) {
        c.vars({ "x" });
        c.values(x);
        c.CHECK(num8(num16(x)) == x);
        c.CHECK(num8(num32(x)) == x);
        c.CHECK(num8(num64(x)) == x);
        c.CHECK(-x + x == 0);
        c.CHECK(x.floor() == 0 && x.ceil() == 0 /* overflow */);
        c.CHECK(x.frac() == x);
    });

    printf("Testing binary laws on num8...\n");
    success &= runInternalArithmeticTest<num8>(0);

    printf("Testing comparisons of num8 with integers...\n");
    success &= runIntegerComparisonTest<num8>();

    printf("Testing unary laws on num16...\n");
    success &= runWithChecker<num16>([](num16 x, Checker<num16> &c) {
        c.vars({ "x" });
        c.values(x);
        c.CHECK(num16(num8(x)).v == (x.v & 0xff));
        c.CHECK(x < 0 || num16(num8(x)) == x % num16(1));
        c.CHECK(num16(num32(x)) == x);
        c.CHECK(num16(num64(x)) == x);
        c.CHECK(-x + x == 0);
        c.CHECK(x.floor() <= x && ((int)x == num16::maxInt || x <= x.ceil()));
        c.CHECK((x.ceil() - x.floor()) == num16(x != num16((int)x)));
        c.CHECK(x.floor() + x.frac() == x);
    });

    printf("Testing binary laws on num16...\n");
    success &= runInternalArithmeticTest<num16>(1);

    printf("Testing comparisons of num16 with integers...\n");
    success &= runIntegerComparisonTest<num16>();

    printf("Testing unary laws on num32...\n");
    success &= runWithChecker<num32>([](num32 x, Checker<num32> &c) {
        c.vars({ "x" });
        c.values(x);
        c.CHECK(num32(num8(x)).v == (x.v & 0x0000ff00));
        c.CHECK(num32(num16(x)).v >> 8 == (int16_t)(x.v >> 8));
        c.CHECK(num32(num64(x)) == x);
        c.CHECK(-x + x == 0);
        c.CHECK(x.floor() <= x && ((int)x == num32::maxInt || x <= x.ceil()));
        c.CHECK((x.ceil() - x.floor()) == num32(x != num32((int)x)));
        c.CHECK(x.floor() + x.frac() == x);
    });

    printf("Testing binary laws on num32...\n");
    success &= runInternalArithmeticTest<num32>(1);

    printf("Testing comparisons of num32 with integers...\n");
    success &= runIntegerComparisonTest<num32>();

    printf("Testing unary laws on num64...\n");
    success &= runWithChecker<num64>([](num64 x, Checker<num64> &c) {
        c.vars({ "x" });
        c.values(x);
        c.CHECK(num64(num8(x)).v == (x.v & 0x00000000ff000000ll));
        c.CHECK(num64(num16(x)).v >> 24 == (int16_t)(x.v >> 24));
        c.CHECK(num64(num32(x)).v >> 16 == (int32_t)(x.v >> 16));
        c.CHECK(-x + x == num64(0));
        c.CHECK(x.floor() <= x && ((int)x == num64::maxInt || x <= x.ceil()));
        c.CHECK((x.ceil() - x.floor()) == num64(x != num64((int)x)));
        c.CHECK(x.floor() + x.frac() == x);
    });

    return (success ? 0 : 1);
}
