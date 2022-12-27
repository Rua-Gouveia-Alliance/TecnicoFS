#ifndef _BETTER_THREADS_H
#define _BETTER_THREADS_H

#include "betterassert.h"

#define THREAD_CREATE(thread, routine)                                         \
    {                                                                          \
        int m_err = pthread_create(thread, NULL, routine, NULL);               \
        ALWAYS_ASSERT(m_err != -1, "pthread_create critical error")            \
    }

#endif