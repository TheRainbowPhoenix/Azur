//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.defs: General definitions that are included in every file

/* This exposes compile-time configuration symbols. I don't like running the
   risk of using preprocessor conditionals without pulling the configuration,
   and by getting it here, every header will include it. */
#include <azur/config.h>

/* C++ header guards. Azur headers aren't designed for C, but if any is in the
   future then it'll need this. */
#ifdef __cplusplus
# define AZUR_BEGIN_DECLS extern "C" {
# define AZUR_END_DECLS }
#else
# define AZUR_BEGIN_DECLS
# define AZUR_END_DECLS
#endif

/* Common types. */
#ifdef __cplusplus
# include <cinttypes>
# include <cstddef>
# include <cstdarg>
#else
# include <inttypes.h>
# include <stddef.h>
# include <stdbool.h>
# include <stdarg.h>
#endif

/* More common types. */
#include <sys/types.h>

/* Type aliases. */
typedef unsigned int uint;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/* Import fundamental data structures from the standard library */
#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>
using std::string, std::vector, std::map;
#endif

/* Import common numerical types for libnum */
#ifdef __cplusplus
#include <num/num.h>
#include <num/vec.h>
#include <num/rect.h>
using namespace libnum;
#endif
