#ifndef __DEBUG_H
#define __DEBUG_H
#include <bwio.h>


#ifdef PRODUCTION

#define debug(fmt, ...)     ((void)0)
#define assert(ignore)      ((void)0)

#else // ! PRODUCTION

#define debug(fmt, ...)     bwprintf(COM2,  \
    "[%s:%d] %s: " fmt "\r\n",              \
    __FILE__,                               \
    __LINE__,                               \
    __FUNCTION__,                           \
    ## __VA_ARGS__                          \
)

#define assert(expr) (                          \
    (expr) ?                                    \
    (void)(0) :                                 \
    debug("\tASSERTION \"%s\" FAILED!", #expr)  \
)

#endif // PRODUCTION

#endif // __DEBUG_H
