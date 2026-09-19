#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdio.h> // IWYU pragma: keep
#include <string.h>

constexpr int LOG_MSG_LEN = 2024;
constexpr int LOG_TIME_STR_LEN = 32;

enum LOG_LEVEL : uint8_t {
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ALL,
};

extern enum LOG_LEVEL g_log_level;

void log_msg(enum LOG_LEVEL lvl, const char* msg);

#if defined(__clang__) || defined(__GNUC__)
#    define FILENAME __builtin_FILE_NAME()
#else
#    define FILENAME src_file_basename(__FILE__)
#endif

#define LOG(lvl, msg)                                                                  \
    do {                                                                               \
        char _buf[LOG_MSG_LEN];                                                        \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s", FILENAME, __LINE__, __func__, msg);\
        log_msg(lvl, _buf);                                                            \
    } while (0)

#define LOG_MSG(lvl, msg1, msg2)                                                                  \
    do {                                                                                          \
        char _buf[LOG_MSG_LEN];                                                                   \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s: %s", __FILE__, __LINE__, __func__, msg1, msg2);\
        log_msg(lvl, _buf);                                                                       \
    } while (0)

#define LOG_ERRNO(lvl, msg)                                                                                 \
    do {                                                                                                    \
        char _buf[LOG_MSG_LEN];                                                                             \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s: %s", __FILE__, __LINE__, __func__, msg, strerror(errno));\
        log_msg(lvl, _buf);                                                                                 \
    } while (0)

#define LOG_FATAL(msg)                                                                 \
    do {                                                                               \
        char _buf[LOG_MSG_LEN];                                                        \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s", __FILE__, __LINE__, __func__, msg);\
        log_msg(LOG_LEVEL_ERROR, _buf);                                                \
    } while (0)

#define LOG_FATAL_MSG(msg1, msg2)                                                                 \
    do {                                                                                          \
        char _buf[LOG_MSG_LEN];                                                                   \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s: %s", __FILE__, __LINE__, __func__, msg1, msg2);\
        log_msg(LOG_LEVEL_ERROR, _buf);                                                           \
    } while (0)

#endif
