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

/* Quick macros that insert parameters automatically. */
#define azlog(LEVEL, FMT, ...) \
    azur::log::write(azur::log::LogLevel::LEVEL, \
        {__FILE__, __LINE__, __func__}, FMT, ## __VA_ARGS__)

#define azlog_begin() azur::log::output()->beginGroup()
#define azlog_end() azur::log::output()->endGroup()

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

// TODO: Put logger in current app
class LogOutput;
struct LogLocation;
/* Current log output handler; never NULL (defaults to stderr). */
LogOutput *output();

/* Produce a log message. See also azlog() and azlogc() for shorter versions.
   level:  Message level
   loc:   Location of the log call in source code
   fmt...: print()-style format */
void write(LogLevel level, LogLocation const &loc, char const *fmt, ...);

/* Get the minimum-level filter. Messages below the minimum level are filtered
   out. The default is INFO in release builds, DEBUG in debug builds. */
LogLevel minimumLevelFilter();

/* Set the minimum-level filter. */
void setMinimumLevelFilter(LogLevel level);

/* Location in code from which a log message is emitted. */
struct LogLocation
{
    char const *file;       /* File name, usually __FILE__ */
    int line;               /* Line number, usually __LINE__ */
    char const *function;   /* Function name, usually __func__ */
};

/* An output for logged messages (terminal, file, web console, etc). */
class LogOutput
{
public:
    /* Produce a log message with a format and a `va_list`.
       level: Message level
       loc:   Location of the log call in source code
       fmt:   print()-style format
       args:  List of arguments for fmt*/
    virtual void writev(LogLevel level, LogLocation const &loc,
                        char const *fmt, va_list args) = 0;

    /* Produce a log message with a format and variadic arguments. */
    void write(LogLevel level, LogLocation const &loc, char const *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        writev(level, loc, fmt, args);
        va_end(args);
    }

    /* Begin or end a message group. Messages between calls to `beginGroup()`
       and `endGroup()` should be treated as one. Typically, the level and
       location prefix is only shown for the first message. Outputs might give
       additional semantics to groups. Groups are a convenience for splitting
       unitary log messages into multiple calls and cannot be nested. */
    virtual void beginGroup() = 0;
    virtual void endGroup() = 0;
};

/* Terminal and file output for logged messages. */
class LogOutputFile: public LogOutput
{
public:
    LogOutputFile(FILE *fp, bool closeFilePointer, bool enableAnsiEscapes);
    ~LogOutputFile();

    void writev(LogLevel level, LogLocation const &loc, char const *fmt,
                va_list args) override;
    void beginGroup() override;
    void endGroup() override;

private:
    /* File pointer to output stream */
    FILE *m_fp;
    /* Whether the stream should be closed when destroying the logger */
    bool m_closeFilePointer;
    /* Whether to enable ANSI escapes-based colors */
    bool m_ansiEscapes;
    /* Size of the current group; -1 when not in a group. */
    int m_groupSize = -1;

    /* Whether we're currently in a group. */
    bool inGroup() { return m_groupSize >= 0; }
};

#if AZUR_PLATFORM_EMSCRIPTEN

/* Web output in emscripten using the Javascript console API. */
class LogOutputJSConsole: public LogOutput
{
public:
    void writev(LogLevel level, LogLocation const &loc, char const *fmt,
                va_list args) override;
    void beginGroup() override;
    void endGroup() override;

private:
    /* String accumulated so for for messages in a group. */
    string m_groupString;
    /* Size of the current group; -1 when not in a group. */
    int m_groupSize = -1;
    /* Log level of the current group; undefined when not in a group. */
    LogLevel m_groupLevel;
    /* Prefix string of the current group; empty when not in a group. */
    string m_groupPrefix;

    /* Whether we're currently in a group. */
    bool inGroup() { return m_groupSize >= 0; }
    /* Prefix string for a level and location. */
    string prefixString(LogLevel level, LogLocation const &loc);
    /* Output to console. */
    void output(LogLevel level, string const &prefix, char const *data);
};

#endif

// TODO: LogOutputBuffer class with a rotating buffer, to display logs in GUI
// TODO: Application-specific type integer in logs API

} /* namespace azur::log */
