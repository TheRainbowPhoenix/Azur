//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//

#include <azur/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <azur/ecs.h>

#if AZUR_PLATFORM_EMSCRIPTEN
# include <emscripten.h>
#endif

namespace azur::log {

#if AZUR_DEBUG
 static LogLevel levelFilter = LogLevel::DEBUG;
#else
 static LogLevel levelFilter = LogLevel::INFO;
#endif

#define ANSI_RED        "\e[31m"
#define ANSI_CYAN       "\e[36m"
#define ANSI_YELLOW     "\e[33m"
#define ANSI_CLEAR      "\e[0m"

#if AZUR_PLATFORM_EMSCRIPTEN
static LogOutputJSConsole LOJSC;
static LogOutput *LO = &LOJSC;
#else
static LogOutputFile LOF(stderr, false, (bool)AZUR_TERMINAL_ANSI);
static LogOutput *LO = &LOF;
#endif

LogOutput *output()
{
    return LO;
}

void write(LogLevel level, LogLocation const &loc, char const *fmt, ...)
{
    if(level < levelFilter)
        return;

    va_list args;
    va_start(args, fmt);
    output()->writev(level, loc, fmt, args);
    va_end(args);

    if(level == LogLevel::FATAL)
        exit(1);
}

LogLevel minimumLevelFilter()
{
    return levelFilter;
}

void setMinimumLevelFilter(LogLevel lv)
{
    levelFilter = lv;
}

//=== File/terminal output ===================================================//

LogOutputFile::LogOutputFile(
    FILE *fp, bool closeFilePointer, bool enableAnsiEscapes):
    m_fp {fp}, m_closeFilePointer {closeFilePointer},
    m_ansiEscapes {enableAnsiEscapes}
{}

LogOutputFile::~LogOutputFile()
{
    if(m_closeFilePointer)
        fclose(m_fp);
    m_fp = nullptr;
}

void LogOutputFile::writev(
    LogLevel level, LogLocation const &loc, char const *fmt, va_list args)
{
    char const *RED     = m_ansiEscapes ? ANSI_RED : "";
    char const *CYAN    = m_ansiEscapes ? ANSI_CYAN : "";
    char const *YELLOW  = m_ansiEscapes ? ANSI_YELLOW : "";
    char const *CLEAR   = m_ansiEscapes ? ANSI_CLEAR : "";

    /* Skip the prefix if this is a group and not the first message */
    if(!inGroup() || m_groupSize == 0) {
        if(level == LogLevel::DEBUG)
            fprintf(m_fp, "%s%s:%d:", CYAN, loc.file, loc.line);

        fprintf(m_fp, "%s%s:%s ", YELLOW, loc.function, CLEAR);

        if(level == LogLevel::ERROR)
            fprintf(m_fp, "%serror:%s ", RED, CLEAR);
        if(level == LogLevel::FATAL)
            fprintf(m_fp, "%sfatal error:%s ", RED, CLEAR);
    }

    vfprintf(m_fp, fmt, args);

    // TODO: Exiting on fatal error isn't the responsibility of LogOutput
    if(level == LogLevel::FATAL)
        exit(1);

    m_groupSize += inGroup();
}

void LogOutputFile::beginGroup()
{
    m_groupSize = 0;
}

void LogOutputFile::endGroup()
{
    m_groupSize = -1;
}

//=== emscripten web console output ==========================================//

#if AZUR_PLATFORM_EMSCRIPTEN

void LogOutputJSConsole::writev(
    LogLevel level, LogLocation const &loc, char const *fmt, va_list args)
{
    char *data = NULL;
    char const *fallback = "<vasprintf error>\n";
    vasprintf(&data, fmt, args);

    /* For messages outside of a group, print directly */
    if(!inGroup()) {
        output(level, prefixString(level, loc), data ? data : fallback);
    }
    else {
        /* For the first message of a group, record group info */
        if(m_groupSize == 0) {
            m_groupPrefix = prefixString(level, loc);
            m_groupLevel = level;
        }

        /* Accumulate message data */
        m_groupString += (data ? data : fallback);
        m_groupSize++;
    }

    free(data);

    // TODO: Exiting on fatal error isn't the responsibility of LogOutput
    if(level == LogLevel::FATAL)
        exit(1);
}

void LogOutputJSConsole::beginGroup()
{
    m_groupSize = 0;
    m_groupString = "";
}

void LogOutputJSConsole::endGroup()
{
    if(m_groupSize > 0)
        output(m_groupLevel, m_groupPrefix, m_groupString.c_str());

    m_groupSize = -1;
    m_groupString = "";
}

std::string LogOutputJSConsole::prefixString(
    LogLevel level, LogLocation const &loc)
{
    char str[32];

    std::string prefix = "%c"; /* cyan */
    if(level == LogLevel::DEBUG) {
        prefix += loc.file;
        snprintf(str, sizeof str, ":%d: ", loc.line);
        prefix += str;
    }

    prefix += "%c"; /* gray */
    prefix += loc.function ? loc.function : "<anonymous>";
    prefix += ":";

    prefix += "%c"; /* normal */
    if(level == LogLevel::FATAL)
        prefix += " FATAL ERROR: ";
    return prefix;
}

/* All prefixes follow the same generic format for colors with a fixed number
   of color changes (%c); this avoids variations in the number of parameters.
     <debug>: <function>: <message>
     ^cyan    ^gray       ^normal */
#define CONSOLE_CALL(NAME, PREFIX, MESSAGE) \
    EM_ASM({ \
        console.NAME(UTF8ToString($0), 'color:#21797d', 'color:#606060', ' ', \
                     UTF8ToString($1)); \
    }, PREFIX, MESSAGE)

void LogOutputJSConsole::output(
    LogLevel level, std::string const &prefix, char const *message)
{
    switch(level) {
    case LogLevel::ERROR:
    case LogLevel::FATAL:
        CONSOLE_CALL(error, prefix.c_str(), message);
        break;
    case LogLevel::WARN:
        CONSOLE_CALL(warn, prefix.c_str(), message);
        break;
    case LogLevel::INFO:
        CONSOLE_CALL(info, prefix.c_str(), message);
        break;
    case LogLevel::DEBUG:
        CONSOLE_CALL(debug, prefix.c_str(), message);
        break;
    default:
        CONSOLE_CALL(log, prefix.c_str(), message);
    }
}

#endif

} /* namespace azur::log */
