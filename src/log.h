#ifndef LOG_H
#define LOG_H

#include <errno.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdio.h> // IWYU pragma: keep
#include <string.h>// IWYU pragma: keep

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

#ifdef __cplusplus
extern "C" {
#endif

void log_msg(enum LOG_LEVEL lvl, const char* msg);

#ifdef __cplusplus
}
#endif

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

#define LOG_DEBUG(msg) LOG(LOG_LEVEL_DEBUG, msg)
#define LOG_INFO(msg) LOG(LOG_LEVEL_INFO, msg)
#define LOG_WARN(msg) LOG(LOG_LEVEL_WARN, msg)
#define LOG_ERROR(msg) LOG(LOG_LEVEL_ERROR, msg)

#define LOG_VK_DEBUG(msg) log_msg(LOG_LEVEL_DEBUG, msg)
#define LOG_VK_INFO(msg) log_msg(LOG_LEVEL_INFO, msg)
#define LOG_VK_WARN(msg) log_msg(LOG_LEVEL_WARN, msg)
#define LOG_VK_ERROR(msg) log_msg(LOG_LEVEL_ERROR, msg)

#define LOG_MSG(lvl, msg1, msg2)                                                                  \
    do {                                                                                          \
        char _buf[LOG_MSG_LEN];                                                                   \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s: %s", FILENAME, __LINE__, __func__, msg1, msg2);\
        log_msg(lvl, _buf);                                                                       \
    } while (0)

#define LOG_ERRNO(lvl, msg)                                                                                 \
    do {                                                                                                    \
        char _buf[LOG_MSG_LEN];                                                                             \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s: %s", FILENAME, __LINE__, __func__, msg, strerror(errno));\
        log_msg(lvl, _buf);                                                                                 \
    } while (0)

#define LOG_FATAL(msg)                                                                 \
    do {                                                                               \
        char _buf[LOG_MSG_LEN];                                                        \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s", FILENAME, __LINE__, __func__, msg);\
        log_msg(LOG_LEVEL_ERROR, _buf);                                                \
    } while (0)

#define LOG_FATAL_MSG(msg1, msg2)                                                                 \
    do {                                                                                          \
        char _buf[LOG_MSG_LEN];                                                                   \
        snprintf(_buf, sizeof(_buf), "%s:%d:%s %s: %s", FILENAME, __LINE__, __func__, msg1, msg2);\
        log_msg(LOG_LEVEL_ERROR, _buf);                                                           \
    } while (0)

#endif
