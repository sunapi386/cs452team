#ifndef __DEBUG_H
#define __DEBUG_H
#include <bwio.h>


#ifdef PRODUCTION

#define error(fmt, ...)     (void)(0)

#else // ! PRODUCTION

#define error(fmt, ...)     bwprintf(COM2, \
    "%s @ %s - %d: " fmt "\r\n",
    __FUNCTION__,
    __FILE__,
    __LINE,
    ## __VA_ARGS__
)

#endif // PRODUCTION

#endif // __DEBUG_H
