#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

constexpr int B_LOG_MSG_LEN = 2024;
constexpr int B_LOG_TIME_STR_MAX_LEN = 32;

enum B_LOG_LEVEL : uint8_t {
    B_FATAL,
    B_ERROR,
    B_WARN,
    B_INFO,
    B_DEBUG,
    B_ALL,
};

extern enum B_LOG_LEVEL g_log_level;

void b_log(enum B_LOG_LEVEL lvl, const char* msg);

#if defined(__clang__) || defined(__GNUC__)
#    define B_FILENAME __builtin_FILE_NAME()
#else
#    define B_FILENAME b_file_basename(__FILE__)
#endif

#define B_LOG(lvl, msg)                                                                      \
    do {                                                                                     \
        char log_buf[B_LOG_MSG_LEN];                                                         \
        snprintf(log_buf, sizeof(log_buf), "%s:%d:%s %s", B_FILENAME, __LINE__, __func__, msg);\
        b_log(lvl, log_buf);                                                                 \
    } while (0)

#define B_LOG_MSG(lvl, msg1, msg2)                                                                      \
    do {                                                                                                \
        char log_buf[B_LOG_MSG_LEN];                                                                    \
        snprintf(log_buf, sizeof(log_buf), "%s:%d:%s %s: %s", __FILE__, __LINE__, __func__, msg1, msg2);\
        b_log(lvl, log_buf);                                                                            \
    } while (0)

#define B_LOG_ERRNO(lvl, msg)                                                                                     \
    do {                                                                                                          \
        char log_buf[B_LOG_MSG_LEN];                                                                              \
        snprintf(log_buf, sizeof(log_buf), "%s:%d:%s %s: %s", __FILE__, __LINE__, __func__, msg, strerror(errno));\
        b_log(lvl, log_buf);                                                                                      \
    } while (0)

#define B_LOG_FATAL(msg)                                                                     \
    do {                                                                                     \
        char log_buf[B_LOG_MSG_LEN];                                                         \
        snprintf(log_buf, sizeof(log_buf), "%s:%d:%s %s", __FILE__, __LINE__, __func__, msg);\
        b_log(B_FATAL, log_buf);                                                             \
    } while (0)

#define B_LOG_FATAL_MSG(msg1, msg2)                                                                     \
    do {                                                                                                \
        char log_buf[B_LOG_MSG_LEN];                                                                    \
        snprintf(log_buf, sizeof(log_buf), "%s:%d:%s %s: %s", __FILE__, __LINE__, __func__, msg1, msg2);\
        b_log(B_FATAL, log_buf);                                                                        \
    } while (0)

#endif
