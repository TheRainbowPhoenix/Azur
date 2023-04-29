//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// num.primitives: Platform-specific optimized primitives
//
// This header provides a generic interface to optimized primitives such as
// unusual-size divisions. These are used in libnum functions for a performance
// boost.
//
// The following functions are defined:
// - div_X_Y_Z: Division of an X by a Y with a result of type Z (where X,Y,Z is
//              a signed integer type iN or an unsigned integer type uN)
//---

#pragma once

#include <num/config.h>
#include <cstdint>

namespace libnum::prim {

#ifdef LIBNUM_ARCH_SH4ALDSP

extern int div_i32_i16_i16(int32_t num, int16_t denum);

#else

static inline constexpr int div_i32_i16_i16(int32_t num, int16_t denum)
{
    return num / denum;
}

#endif

} /* namespace libnum::prim */
