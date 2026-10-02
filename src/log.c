#include "log.h"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum LOG_LEVEL g_log_level = LOG_LEVEL_ALL;

#define CONSOLE_RED "\033[31m"
#define CONSOLE_GREEN "\033[32m"
#define CONSOLE_YELLOW "\033[33m"
#define CONSOLE_WHITE "\033[37m"
#define CONSOLE_GRAY "\033[90m"
#define CONSOLE_BOLD "\033[1m"
#define CONSOLE_RESET "\033[0m"

#define CONSOLE_FATAL "\x1b[1m\x1b[37m\x1b[41m"

static char* log_level_to_string(enum LOG_LEVEL log_lvl)
{
    switch (log_lvl) {
        case LOG_LEVEL_FATAL:
            return "FATAL";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        case LOG_LEVEL_WARN:
            return "WARN ";
        case LOG_LEVEL_INFO:
            return "INFO ";
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        default:
            return "";
    }
}

static enum LOG_LEVEL log_string_to_level(const char* log_lvl)
{
    if (strcmp("FATAL", log_lvl) == 0) {
        return LOG_LEVEL_FATAL;
    }
    if (strcmp("ERROR", log_lvl) == 0) {
        return LOG_LEVEL_ERROR;
    }
    if (strcmp("WARN", log_lvl) == 0) {
        return LOG_LEVEL_WARN;
    }
    if (strcmp("INFO", log_lvl) == 0) {
        return LOG_LEVEL_INFO;
    }
    if (strcmp("DEBUG", log_lvl) == 0) {
        return LOG_LEVEL_DEBUG;
    }
    return LOG_LEVEL_ALL;
}

static enum LOG_LEVEL log_level_set(void)
{
#ifdef _WIN32
    char* log_lvl = NULL;
    size_t len = 0;
    if (_dupenv_s(&log_lvl, &len, "APP_LOG") != 0 || log_lvl == NULL) {
        return LOG_LEVEL_ALL;
    }
    const enum LOG_LEVEL lvl = log_string_to_level(log_lvl);
    free(log_lvl);
    return lvl;
#else
    const char* log_lvl = getenv("APP_LOG");
    if (log_lvl) {
        return log_string_to_level(log_lvl);
    }
    return LOG_LEVEL_ALL;
#endif
}

static const char* log_color_by_level(enum LOG_LEVEL log_lvl)
{
    switch (log_lvl) {
        case LOG_LEVEL_FATAL:
            return CONSOLE_FATAL;
        case LOG_LEVEL_ERROR:
            return CONSOLE_RED;
        case LOG_LEVEL_WARN:
            return CONSOLE_YELLOW;
        case LOG_LEVEL_INFO:
            return CONSOLE_WHITE;
        case LOG_LEVEL_DEBUG:
            return CONSOLE_GRAY;
        default:
            return CONSOLE_RESET;
    }
}

static struct tm log_local_time_get()
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
static int log_console_colors_enable(void)
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

void log_msg(enum LOG_LEVEL lvl, const char* msg)
{
    if (lvl > g_log_level) {
        return;
    }

    const struct tm local_time = log_local_time_get();

    const int colorized = 
#ifdef _WIN32
        log_console_colors_enable();
#else
        1;
#endif

    const char* time_color = colorized ? CONSOLE_GRAY : "";
    const char* color = colorized ? log_color_by_level(lvl) : "";
    const char* color_reset = colorized ? CONSOLE_RESET : "";

    const char* lvl_str = log_level_to_string(lvl);

    char time_string[LOG_TIME_STR_LEN];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &local_time);

    char line[LOG_MSG_LEN];
    snprintf(
        line, sizeof(line), "%s%s%s %s%s %s%s\n", time_color, time_string, color_reset, color, lvl_str, msg,
        color_reset);

    printf("%s", line);
}
