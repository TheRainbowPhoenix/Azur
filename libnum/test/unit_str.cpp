//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// unit_str.cpp: Unit tests for string representations

#include <stdio.h>
#include <string.h>
#include "unit_sample.h"
#include "unit_check.h"

#define CHECK(EXPR, MSG) check(EXPR, #MSG)
#define CHECK_STR(value, exp) \
    CHECK(({ value.strToBuffer(_str); !strcmp(_str, exp); }), value: exp)

bool runBasicTest(void)
{
    char _str[128];
    Checker c;

    c.CHECK_STR(num8(0), "0");
    c.CHECK_STR(num16(1), "1");
    c.CHECK_STR(num16(-1), "-1");
    c.CHECK_STR(num16(-0.25), "-0.25");
    c.CHECK_STR(num8(0.5), "0.5");
    c.CHECK_STR(num8(0.25), "0.25");
    c.CHECK_STR(num32(0.53125), "0.53125");
    c.CHECK_STR(num32(-73), "-73");
    c.CHECK_STR(num32(-5329), "-5329");
    c.CHECK_STR(num32(-32767), "-32767");
    c.CHECK_STR(num8(0.99609375), "0.99609375");
    c.CHECK_STR(num16(-127), "-127");
    c.CHECK_STR(num16(-30.6015625), "-30.6015625");
    c.CHECK_STR(num16(-128), "-128"); /* Overflow on -128 --> 128 after sign */

    num64 x64;
    x64.v = 1;
    c.CHECK_STR(x64, "0.00000000023283064365386962890625");
    x64.v = -1;
    c.CHECK_STR(x64, "-0.00000000023283064365386962890625");

    return c.successful();
}

int main(void)
{
    bool success = true;

    printf("Testing a handful of pre-written inputs...\n");
    success &= runBasicTest();

    return (success ? 0 : 1);
}
