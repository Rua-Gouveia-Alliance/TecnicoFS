#ifndef _BETTER_PIPES_H
#define _BETTER_PIPES_H

#include "betterassert.h"

#define MK_FIFO(path)                                                          \
    {                                                                          \
        unlink(path);                                                          \
        int m_err = mkfifo(path, 0777);                                        \
        ALWAYS_ASSERT(m_err != -1 || errno == EEXIST, "mkfifo critical error") \
    }

#endif