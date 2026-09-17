#include "log.h"

#ifdef _WIN32
#    include <io.h>
#    include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum B_LOG_LEVEL g_log_level = 0;

#define B_CONSOLE_RED "\033[31m"
#define B_CONSOLE_GREEN "\033[32m"
#define B_CONSOLE_YELLOW "\033[33m"
#define B_CONSOLE_WHITE "\033[37m"
#define B_CONSOLE_GRAY "\033[90m"
#define B_CONSOLE_BOLD  "\033[1m"
#define B_CONSOLE_RESET "\033[0m"

#define B_CONSOLE_FATAL "\x1b[1m\x1b[37m\x1b[41m"

static char* b_log_level_to_string(enum B_LOG_LEVEL log_lvl)
{
    switch (log_lvl) {
        case B_FATAL:
            return "FATAL";
        case B_ERROR:
            return "ERROR";
        case B_WARN:
            return "WARN ";
        case B_INFO:
            return "INFO ";
        case B_DEBUG:
            return "DEBUG";
        default:
            return "";
    }
}

static enum B_LOG_LEVEL b_string_to_log_level(const char* log_lvl)
{
    if (strcmp("FATAL", log_lvl) == 0) {
        return B_FATAL;
    }
    if (strcmp("ERROR", log_lvl) == 0) {
        return B_ERROR;
    }
    if (strcmp("WARN", log_lvl) == 0) {
        return B_WARN;
    }
    if (strcmp("INFO", log_lvl) == 0) {
        return B_INFO;
    }
    if (strcmp("DEBUG", log_lvl) == 0) {
        return B_DEBUG;
    }
    return B_ALL;
}

static enum B_LOG_LEVEL b_log_level_set(void)
{
#ifdef _WIN32
    char* log_lvl = NULL;
    size_t len = 0;
    if (_dupenv_s(&log_lvl, &len, "U_LOG") != 0 || log_lvl == NULL) {
        return B_ALL;
    }
    const enum B_LOG_LEVEL lvl = b_string_to_log_level(log_lvl);
    free(log_lvl);
    return lvl;
#else
    const char* log_lvl = getenv("U_LOG");   /* POSIX path unchanged */
    if (log_lvl) {
        return b_string_to_log_level(log_lvl);
    }
    return B_ALL;
#endif
}

static const char* b_color_by_log_level(enum B_LOG_LEVEL log_lvl)
{
    switch (log_lvl) {
        case B_FATAL:
            return B_CONSOLE_FATAL;
        case B_ERROR:
            return B_CONSOLE_RED;
        case B_WARN:
            return B_CONSOLE_YELLOW;
        case B_INFO:
            return B_CONSOLE_WHITE;
        case B_DEBUG:
            return B_CONSOLE_GRAY;
        default:
            return B_CONSOLE_RESET;
    }
}

static struct tm b_get_local_time()
{
    const time_t utc_sec = time(nullptr);

    struct tm local_time;
#ifdef _WIN32
    localtime_s(&local_time, &utc_sec);
#else
    localtime_r(&utc_sec, &local_time);
#endif

    return local_time;
}

#ifdef _WIN32
static int b_console_enable_colors(void)
{
    static int state = -1;
    if (state != -1) {
        return state;
    }

    const HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE || !_isatty(_fileno(stdout))) {
        state = 0;
        return 0;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) {
        state = 0;
        return 0;
    }

    if (SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        state = 1;
        return 1;
    }

    state = 0;
    return 0;
}
#endif

void b_log(enum B_LOG_LEVEL lvl, const char* msg)
{
    const struct tm local_time = b_get_local_time();
    const int colorized =
#ifdef _WIN32
        b_console_enable_colors();
#else
        1;
#endif

    const char* color = colorized ? b_color_by_log_level(lvl) : "";
    const char* reset = colorized ? B_CONSOLE_RESET : "";

    const char* lvl_str = b_log_level_to_string(lvl);

    char time_string[B_LOG_TIME_STR_MAX_LEN];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &local_time);

    char line[B_LOG_MSG_LEN];
    snprintf(line, sizeof(line), "%s %s%s %s%s\n", time_string, color, lvl_str, msg, reset);

    printf("%s", line);
}
