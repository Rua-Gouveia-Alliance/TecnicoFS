#ifndef __UTILS_BETTER_ASSERT_H__
#define __UTILS_BETTER_ASSERT_H__

#include "logging.h"
#include <stdbool.h>
#include <stdio.h>

#define ALWAYS_ASSERT(CONDEXPR, ...)                                           \
    do {                                                                       \
        if (!(CONDEXPR)) {                                                     \
            PANIC(__VA_ARGS__);                                                \
        }                                                                      \
    } while (0);

#endif // __UTILS_BETTER_ASSERT_H__
