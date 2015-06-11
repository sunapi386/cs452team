#ifndef __DEBUG_H
#define __DEBUG_H
#include <bwio.h>


#ifdef PRODUCTION

#define debug(fmt, ...)     ((void)0)
#define assert(ignore)      ((void)0)

#else // ! PRODUCTION

#define debug(fmt, ...)                     \
    bwprintf(COM2,                          \
    "[%s:%d] %s: " fmt "\r\n",              \
    __FILE__,                               \
    __LINE__,                               \
    __FUNCTION__,                           \
    ## __VA_ARGS__                          \
)

static void __assert_fail__ (__const char *__expr,
                             __const char *__file,
                             __const unsigned int __line,
                             __const char *__func)
__attribute__((always_inline, noreturn));

static void __assert_fail__ (__const char *__expr,
                             __const char *__file,
                             __const unsigned int __line,
                             __const char *__func) {
    bwsetfifo(COM2, OFF);
    bwsetspeed(COM2, 115200);
    bwprintf(COM2, "Assertion %s failed at [%s:%d] %s\r\n",
        __expr, __file, __line, __func);
    while(1) {
        asm volatile("nop");
    }
}

#define assert(expr) (                          \
    (expr) ?                                    \
    (void)(0) :                                 \
    __assert_fail__(#expr, __FILE__, __LINE__, __FUNCTION__)  \
)

#endif // PRODUCTION

#endif // __DEBUG_H
