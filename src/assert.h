#ifndef ASSERT_H
#define ASSERT_H

#include <stdlib.h> // IWYU pragma: keep

#include "log.h"

#define ASSERT(expr)           \
    do {                       \
        if (!(expr)) {         \
            LOG_FATAL(#expr);  \
            exit(EXIT_FAILURE);\
        }                      \
    } while (0)

#define ASSERT_MSG(expr, msg)          \
    do {                               \
        if (!(expr)) {                 \
            LOG_FATAL_MSG(#expr, #msg);\
            exit(EXIT_FAILURE);        \
        }                              \
    } while (0)

#define ASSERT_VK(expr)            \
    do {                           \
        if ((expr) != VK_SUCCESS) {\
            LOG_FATAL(#expr);      \
            exit(EXIT_FAILURE);    \
        }                          \
    } while (0)

#define ASSERT_SDL(expr)                         \
    do {                                         \
        if (!(expr)) {                           \
            LOG_FATAL_MSG(#expr, SDL_GetError());\
            exit(EXIT_FAILURE);                  \
        }                                        \
    } while (0)

#endif // ASSERT_H
