#include <azur/log.h>
#include <stdio.h>

#ifdef AZUR_DEBUG
static int _level = AZLOG_DEBUG;
#else
static int _level = AZLOG_INFO;
#endif

#ifdef AZUR_TERMINAL_ANSI
# define RED "\e[31m"
# define CYAN "\e[36m"
# define YELLOW "\e[33m"
# define STOP "\e[0m"
#else /* AZUR_TERMINAL_PLAIN, by default */
# define RED
# define CYAN
# define YELLOW
# define STOP
#endif

int azlog_level(void)
{
    return _level;
}

void azlog_set_level(int level)
{
    _level = level;
}

void azlog_write(int level, char const *file, int line, char const *func,
    bool cont, char const *fmt, ...)
{
    if(level < _level) return;

    if(!cont) {
        #ifdef AZUR_DEBUG
        fprintf(stderr, CYAN "%s:%d: " STOP YELLOW "%s: " STOP,
            file, line, func);
        #else
        (void)file;
        (void)line;
        fprintf(stderr, "%s: ", func);
        #endif

        if(level == AZLOG_ERROR)
            fprintf(stderr, RED "error: " STOP);
        if(level == AZLOG_FATAL)
            fprintf(stderr, RED "fatal error: " STOP);
    }

    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);
}
