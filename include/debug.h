#ifndef __DEBUG_H
#define __DEBUG_H
#include <bwio.h>
#include <utils.h>

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
    bwprintf(COM2, "Assertion %s failed at [%s:%d] %s\r\n",
        __expr, __file, __line, __func);
    for (;;);
}

static inline void __user__assert_fail__ (__const char *__expr,
                                          __const char *__file,
                                          __const unsigned int __line,
                                          __const char *__func) {
    printf(COM2, "Assertion %s failed at [%s:%d] %s\r\n",
        __expr, __file, __line, __func);
    for (;;);
}

#define assert(expr) (                          \
    (expr) ?                                    \
    (void)(0) :                                 \
    __assert_fail__(#expr, __FILE__, __LINE__, __FUNCTION__)  \
)

#define uassert(expr) (                         \
    (expr) ?                                    \
    (void)(0) :                                 \
    __user__assert_fail__(#expr, __FILE__, __LINE__, __FUNCTION__)  \
)


#endif // PRODUCTION

#endif // __DEBUG_H
