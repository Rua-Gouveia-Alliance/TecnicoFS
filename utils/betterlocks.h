#ifndef _BETTER_LOCKS_H
#define _BETTER_LOCKS_H

#include "betterassert.h"

#define MUTEX_INIT(mutex)                                                      \
    {                                                                          \
        int m_err = pthread_mutex_init(mutex, 0);                              \
        ALWAYS_ASSERT(m_err != -1, "pthread_mutex_init critical error")        \
    }

#define MUTEX_DESTROY(mutex)                                                   \
    {                                                                          \
        int m_err = pthread_mutex_destroy(mutex);                              \
        ALWAYS_ASSERT(m_err != -1, "pthread_mutex_destroy critical error")     \
    }

#define MUTEX_LOCK(mutex)                                                      \
    {                                                                          \
        int m_err = pthread_mutex_lock(mutex);                                 \
        ALWAYS_ASSERT(m_err != -1, "pthread_mutex_lock critical error")        \
    }

#define MUTEX_UNLOCK(mutex)                                                    \
    {                                                                          \
        int m_err = pthread_mutex_unlock(mutex);                               \
        ALWAYS_ASSERT(m_err != -1, "pthread_mutex_unlock critical error")      \
    }

#define RWLOCK_INIT(rwlock)                                                    \
    {                                                                          \
        int m_err = pthread_rwlock_init(rwlock, 0);                            \
        ALWAYS_ASSERT(m_err != -1, "pthread_rwlock_init critical error")       \
    }

#define RWLOCK_DESTROY(rwlock)                                                 \
    {                                                                          \
        int m_err = pthread_rwlock_destroy(rwlock);                            \
        ALWAYS_ASSERT(m_err != -1, "pthread_rwlock_destroy critical error")    \
    }

#define RWLOCK_RDLOCK(rwlock)                                                  \
    {                                                                          \
        int m_err = pthread_rwlock_rdlock(rwlock);                             \
        ALWAYS_ASSERT(m_err != -1, "pthread_rwlock_rdlock critical error")     \
    }

#define RWLOCK_WRLOCK(rwlock)                                                  \
    {                                                                          \
        int m_err = pthread_rwlock_wrlock(rwlock);                             \
        ALWAYS_ASSERT(m_err != -1, "pthread_rwlock_wrlock critical error")     \
    }

#define RWLOCK_UNLOCK(rwlock)                                                  \
    {                                                                          \
        int m_err = pthread_rwlock_unlock(rwlock);                             \
        ALWAYS_ASSERT(m_err != -1, "pthread_rwlock_unlock critical error")     \
    }

#define COND_INIT(cond)                                                        \
    {                                                                          \
        int m_err = pthread_cond_init(cond, NULL);                             \
        ALWAYS_ASSERT(m_err != -1, "pthread_cond_init critical error")         \
    }

#define COND_DESTROY(cond)                                                     \
    {                                                                          \
        int m_err = pthread_cond_destroy(cond);                                \
        ALWAYS_ASSERT(m_err != -1, "pthread_cond_destroy critical error")      \
    }

#endif