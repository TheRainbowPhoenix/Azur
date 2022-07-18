//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.log: Logging utilities
//
// This header provides the basic logging utilities. Logs are emitted with the
// azlog() macro, then filtered by level and printed to file.
//
// The main function is the azlog() macro, which takes a level parameter and a
// printf()-style format:
//
//   azlog(ERROR, "rc = %d\n", rc);
//   azlog(INFO, "Game saved!\n");
//---

#pragma once
#include <azur/defs.h>
#include <stdio.h>

namespace azur::log {

/* DEBUG:  Disabled by default unless debug build; includes source file/line
   INFO:   General information that is meaningful to the user (not developer)
   WARN:   Unexpected behavior but program can still keep going
   ERROR:  Erroneous behavior, abandoning some tasks
   FATAL:  Complete panic; provokes exit(1); highest priority */
enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

/* Coloring options.
   PLAIN:  No escape sequences; suitable for files and plain terminals.
   ANSI:   ANSI escape sequences; suitable for terminals. */
enum class LogStyle { PLAIN, ANSI };

/* Produce a log message. See also azlog() and azlogc() for shorter versions.
   level:  Message level
   file:   File name (normally __FILE__)
   line:   Line number (normally __LINE__)
   func:   Function name (normally __func__)
   cont:   Whether this is the continuation of a previous message
   fmt...: print()-style format */
void write(LogLevel level, char const *file, int line, char const *func,
    bool cont, char const *fmt, ...);

/* Get the minimum-level filter. Messages below the minimum level are filtered
   out. The default is INFO in release builds, DEBUG in debug builds. */
LogLevel minimumLevelFilter();

/* Set the minimum-level filter. */
void setMinimumLevelFilter(LogLevel level);

/* Get the output stream for log messages. Default is stderr. */
FILE *outputStream();

/* Set the output stream for log messages. Since the library might output
   messages until very late in execution, a valid output stream should always
   be specified. When using a file, stderr should be set as the output stream
   before closing the file at the end of execution. */
void setOutputStream(FILE *fp, LogStyle style);

} /* namespace azur::log */

/* Quick macros that insert parameters automatically. */
#define azlog(LEVEL, FMT, ...) \
    azur::log::write(azur::log::LogLevel::LEVEL, __FILE__, __LINE__, \
        __func__, false, FMT, ## __VA_ARGS__)
#define azlogc(LEVEL, FMT, ...) \
    azur::log::write(azur::log::LogLevel::LEVEL, __FILE__, __LINE__, \
        __func__, true, FMT, ## __VA_ARGS__)
