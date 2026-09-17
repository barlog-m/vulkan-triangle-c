#include "log.h"

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

#define B_CONSOLE_FATAL B_CONSOLE_RED B_CONSOLE_BOLD

static char* b_log_level_to_string(enum B_LOG_LEVEL log_lvl)
{
    switch (log_lvl) {
        case B_FATAL:
            return "FATAL";
        case B_ERROR:
            return "ERROR";
        case B_WARN:
            return "WARN";
        case B_INFO:
            return "INFO";
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

static enum B_LOG_LEVEL b_log_level_set()
{
    const char* log_lvl = getenv("U_LOG");
    if (log_lvl) {
        return b_string_to_log_level(log_lvl);
    }

    return B_ALL;
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
    localtime_r(&utc_sec, &local_time);

    return local_time;
}

void b_log(enum B_LOG_LEVEL lvl, const char* msg)
{
    const struct tm local_time = b_get_local_time();
    const auto color = b_color_by_log_level(lvl);

    const char* lvl_str = b_log_level_to_string(lvl);

    char time_string[B_LOG_TIME_STR_MAX_LEN];
    strftime(time_string, sizeof(time_string), "%F %H:%M:%S", &local_time);

    char line[B_LOG_MSG_LEN];
    snprintf(line, sizeof(line), "%s %s%s %s%s\n", time_string, color, lvl_str, msg, B_CONSOLE_RESET);

    printf("%s", line);
}
