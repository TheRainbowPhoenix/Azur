//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// unit_static.cpp: Compile-time evaluation tests

#include <num/num.h>
using namespace num;

static_assert(sizeof(num8) == 1);
static_assert(sizeof(num16) == 2);
static_assert(sizeof(num32) == 4);
static_assert(sizeof(num64) == 8);

static_assert(num8(1).v == 0x00);
static_assert(num8(0.5).v == 0x80);
static_assert(num8(0.0625f).v == 0x10);
static_assert((float)num8(0.25) == 0.25f);
static_assert(num8(0.625) + num8(0.125) == num8(0.75));
static_assert(num8(0.25) < num8(0.75));
static_assert(num8(0.5) >= num8(0.5));

static_assert((uint16_t)num16(-1).v == 0xff00);
static_assert(num16(num8(0.25)).v == num16(0.25).v);

static_assert(num32(num16(-15)) == num32(-15));

static_assert(num64(num16(1)) == num64(1));
static_assert(num64(num16(-1)) == num64(-1));

static_assert(num8(0) == 0);
static_assert(num8(0) != 1);
static_assert(num8(0.5) != 0);
static_assert(num8(1) == 0); // overflow

static_assert(!(num8(0) < 0));
static_assert(num8(0) < 1);
static_assert(!(num8(0.5) <= 0));
static_assert(num8(0.5) <= 1);
static_assert(!(num8(0) > 0));
static_assert(num8(0.5) > 0);
static_assert(num8(0) >= 0);
static_assert(!(num8(0.5) >= 1));
