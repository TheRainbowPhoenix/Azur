//---
// azur.log: Logging utilities
//---

#pragma once
#include <azur/defs.h>
AZUR_BEGIN_DECLS

/* Message levels, numbered by "severity". */
enum {
    AZLOG_DEBUG   = 0,
    AZLOG_INFO    = 1,
    AZLOG_WARN    = 2,
    AZLOG_ERROR   = 3,
    AZLOG_FATAL   = 4,
};

/* azlog_level(): Get the current logging level
   Any message with a level at least the current logging level is printed. The
   default logging level is AZLOG_ERROR in release builds, AZLOG_DEBUG in
   development builds. */
int azlog_level(void);

/* azlog_set_level(): Set the current logging level */
void azlog_set_level(int level);

/* azlog(): Write an error message

   This macro produces a line of log at the specified level. The AZLOG_ prefix
   is added automatically by the macro, as well as standard function/line
   information in development builds. */
#define azlog(level, fmt, ...) \
    azlog_write(AZLOG_ ## level, __FILE__, __LINE__, __func__, false, fmt, \
        ## __VA_ARGS__)

/* azlogc(): Continue an error message
   Same as azlog(), but inhibits prefixes, for multi-part messages. */
#define azlogc(level, fmt, ...) \
    azlog_write(AZLOG_ ## level, __FILE__, __LINE__, __func__, true, fmt, \
        ## __VA_ARGS__)

/* Disable output in gint. */
#ifdef AZUR_TERMINAL_NONE
# undef azlog
# undef azlogc
# define azlog(level, fmt, ...)
# define azlogc(level, fmt, ...)
#endif

/* azlog_write(): Support function for azlog() */
void azlog_write(int level, char const *file, int line, char const *func,
    bool cont, char const *fmt, ...);

AZUR_END_DECLS
