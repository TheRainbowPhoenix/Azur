//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//

#include <azur/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

namespace azur::log {

#if AZUR_DEBUG
 static LogLevel levelFilter = LogLevel::DEBUG;
#else
 static LogLevel levelFilter = LogLevel::INFO;
#endif

#if AZUR_TERMINAL_ANSI
 static LogStyle style = LogStyle::ANSI;
#else
 static LogStyle style = LogStyle::PLAIN;
#endif

static FILE *fp = stderr;

#define ANSI_RED        "\e[31m"
#define ANSI_CYAN       "\e[36m"
#define ANSI_YELLOW     "\e[33m"
#define ANSI_CLEAR      "\e[0m"

void write(LogLevel level, char const *file, int line, char const *func,
    bool cont, char const *fmt, ...)
{
    if(level < levelFilter)
        return;

    char const *RED     = ANSI_RED;
    char const *CYAN    = ANSI_CYAN;
    char const *YELLOW  = ANSI_YELLOW;
    char const *CLEAR   = ANSI_CLEAR;

    if(style == LogStyle::PLAIN)
        RED = CYAN = YELLOW = CLEAR = "";

    if(!cont) {
        if(level == LogLevel::DEBUG)
            fprintf(fp, "%s%s:%d:", CYAN, file, line);

        fprintf(fp, "%s%s:%s ", YELLOW, func, CLEAR);

        if(level == LogLevel::ERROR)
            fprintf(fp, "%serror:%s ", RED, CLEAR);
        if(level == LogLevel::FATAL)
            fprintf(fp, "%sfatal error:%s ", RED, CLEAR);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
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

FILE *outputStream()
{
    return fp;
}

void setOutputStream(FILE *new_fp, LogStyle new_style)
{
    fp = new_fp;
    style = new_style;
}

} /* namespace azur::log */
