#ifndef __UTILS_LOGGING_H__
#define __UTILS_LOGGING_H__

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    LOG_QUIET = 0,
    LOG_NORMAL = 1,
    LOG_VERBOSE = 2,
} log_level_t;

void set_log_level(log_level_t level);
extern log_level_t g_level;

#define INFO(...)                                                              \
    do {                                                                       \
        char buf[2048];                                                        \
        snprintf(buf, 2048, __VA_ARGS__);                                      \
        fprintf(stderr, "[INFO]:  %s:%d :: %s :: %s\n", __FILE__, __LINE__,    \
                __func__, buf);                                                \
    } while (0);

#define PANIC(...)                                                             \
    do {                                                                       \
        char buf[2048];                                                        \
        snprintf(buf, 2048, __VA_ARGS__);                                      \
        fprintf(stderr, "[PANIC]: %s:%d :: %s :: %s\n", __FILE__, __LINE__,    \
                __func__, buf);                                                \
        exit(EXIT_FAILURE);                                                    \
    } while (0);

#define WARN(...)                                                              \
    do {                                                                       \
        if (g_level == LOG_NORMAL || g_level == LOG_VERBOSE) {                 \
            char buf[2048];                                                    \
            snprintf(buf, 2048, __VA_ARGS__);                                  \
            fprintf(stderr, "[WARN]:  %s:%d :: %s :: %s\n", __FILE__,          \
                    __LINE__, __func__, buf);                                  \
        }                                                                      \
    } while (0);

#define LOG(...)                                                               \
    do {                                                                       \
        if (g_level == LOG_NORMAL || g_level == LOG_VERBOSE) {                 \
            char buf[2048];                                                    \
            snprintf(buf, 2048, __VA_ARGS__);                                  \
            fprintf(stderr, "[LOG]:   %s:%d :: %s :: %s\n", __FILE__,          \
                    __LINE__, __func__, buf);                                  \
        }                                                                      \
    } while (0);

#define DEBUG(...)                                                             \
    do {                                                                       \
        if (g_level == LOG_VERBOSE) {                                          \
            char buf[2048];                                                    \
            snprintf(buf, 2048, __VA_ARGS__);                                  \
            fprintf(stderr, "[DEBUG]: %s:%d :: %s :: %s\n", __FILE__,          \
                    __LINE__, __func__, buf);                                  \
        }                                                                      \
    } while (0);

#endif // __UTILS_LOGGING_H__
