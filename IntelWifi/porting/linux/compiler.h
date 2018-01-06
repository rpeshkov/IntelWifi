//
//  compiler.h
//  IntelWifi
//
//  Created by Roman Peshkov on 06/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef compiler_h
#define compiler_h

#include <linux/types.h>

#define __READ_ONCE_SIZE                        \
({                                    \
switch (size) {                            \
case 1: *(UInt8 *)res = *(volatile UInt8 *)p; break;        \
case 2: *(UInt16 *)res = *(volatile UInt16 *)p; break;        \
case 4: *(UInt32 *)res = *(volatile UInt32 *)p; break;        \
case 8: *(UInt64 *)res = *(volatile UInt64 *)p; break;        \
default:                            \
os_compiler_barrier();                        \
__builtin_memcpy((void *)res, (const void *)p, size);    \
os_compiler_barrier();                        \
}                                \
})

static inline
void __read_once_size(const volatile void *p, void *res, int size)
{
    __READ_ONCE_SIZE;
}

#ifdef CONFIG_KASAN
/*
 * This function is not 'inline' because __no_sanitize_address confilcts
 * with inlining. Attempt to inline it may cause a build failure.
 *     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67368
 * '__maybe_unused' allows us to avoid defined-but-not-used warnings.
 */
static __no_sanitize_address __maybe_unused
void __read_once_size_nocheck(const volatile void *p, void *res, int size)
{
    __READ_ONCE_SIZE;
}
#else
static inline
void __read_once_size_nocheck(const volatile void *p, void *res, int size)
{
    __READ_ONCE_SIZE;
}
#endif


static inline void __write_once_size(volatile void *p, void *res, int size)
{
    switch (size) {
        case 1: *(volatile UInt8 *)p = *(UInt8 *)res; break;
        case 2: *(volatile UInt16 *)p = *(UInt16 *)res; break;
        case 4: *(volatile UInt32 *)p = *(UInt32 *)res; break;
        case 8: *(volatile UInt64 *)p = *(UInt64 *)res; break;
        default:
            os_compiler_barrier();
            __builtin_memcpy((void *)p, (const void *)res, size);
            os_compiler_barrier();
    }
}


/*
 * Prevent the compiler from merging or refetching reads or writes. The
 * compiler is also forbidden from reordering successive instances of
 * READ_ONCE, WRITE_ONCE and ACCESS_ONCE (see below), but only when the
 * compiler is aware of some particular ordering.  One way to make the
 * compiler aware of ordering is to put the two invocations of READ_ONCE,
 * WRITE_ONCE or ACCESS_ONCE() in different C statements.
 *
 * In contrast to ACCESS_ONCE these two macros will also work on aggregate
 * data types like structs or unions. If the size of the accessed data
 * type exceeds the word size of the machine (e.g., 32 bits or 64 bits)
 * READ_ONCE() and WRITE_ONCE() will fall back to memcpy(). There's at
 * least two memcpy()s: one for the __builtin_memcpy() and then one for
 * the macro doing the copy of variable - '__u' allocated on the stack.
 *
 * Their two major use cases are: (1) Mediating communication between
 * process-level code and irq/NMI handlers, all running on the same CPU,
 * and (2) Ensuring that the compiler does not  fold, spindle, or otherwise
 * mutilate accesses that either do not require ordering or that interact
 * with an explicit memory barrier or atomic instruction that provides the
 * required ordering.
 */

#define __READ_ONCE(x, check)                        \
({                                    \
union { typeof(x) __val; char __c[1]; } __u;            \
if (check)                            \
__read_once_size(&(x), __u.__c, sizeof(x));        \
else                                \
__read_once_size_nocheck(&(x), __u.__c, sizeof(x));    \
__u.__val;                            \
})
#define READ_ONCE(x) __READ_ONCE(x, 1)

/*
 * Use READ_ONCE_NOCHECK() instead of READ_ONCE() if you need
 * to hide memory access from KASAN.
 */
#define READ_ONCE_NOCHECK(x) __READ_ONCE(x, 0)


#define WRITE_ONCE(x, val) \
({                            \
union { typeof(x) __val; char __c[1]; } __u =    \
{ .__val = (__force typeof(x)) (val) }; \
__write_once_size(&(x), __u.__c, sizeof(x));    \
__u.__val;                    \
})



#endif /* compiler_h */
