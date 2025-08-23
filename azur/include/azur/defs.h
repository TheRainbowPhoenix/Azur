//---
// azur.defs: General definitions that are included in every file
//---

/* This exposes compile-time configuration symbols. I don't like running the
   risk of using preprocessor conditionals without pulling the configuration,
   and by getting it here, every header will include it. */
#include <azur/config.h>

/* C++ header guards. */
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
