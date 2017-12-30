//
//  kernel.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef kernel_h
#define kernel_h

#include <sys/errno.h>
#include <libkern/OSTypes.h>
#include <IOKit/IOLib.h>

#define ERFKILL        132    /* Operation not possible due to RF-kill */

#define EHWPOISON    133    /* Memory page has hardware error */


/* Defined for the NFSv3 protocol */
#define EBADHANDLE    521    /* Illegal NFS file handle */
#define ENOTSYNC    522    /* Update synchronization mismatch */
#define EBADCOOKIE    523    /* Cookie is stale */
#define ENOTSUPP    524    /* Operation is not supported */
#define ETOOSMALL    525    /* Buffer or request is too small */
#define ESERVERFAULT    526    /* An untranslatable error occurred */
#define EBADTYPE    527    /* Type not supported by server */
#define EJUKEBOX    528    /* Request initiated, but will not complete before timeout */
#define EIOCBQUEUED    529    /* iocb queued, will get completion event */
#define ERECALLCONFLICT    530    /* conflict with recalled state */



#define WARN_ON(x) (x)
#define STR(x) #x
#define WARN_ON_ONCE(x) (x)

#define unlikely(x) (x)

# define __acquires(x)
# define __releases(x)
# define __acquire(x) (void)0
# define __release(x) (void)0

static inline int fls64(UInt64 x)
{
    int bitpos = -1;
    /*
     * AMD64 says BSRQ won't clobber the dest reg if x==0; Intel64 says the
     * dest reg is undefined if x==0, but their CPU architect says its
     * value is written to set it to the same as before.
     */
    asm("bsrq %1,%q0"
        : "+r" (bitpos)
        : "rm" (x));
    return bitpos + 1;
}


/*
 * Runtime evaluation of get_order()
 */
static inline
int __get_order(unsigned long size)
{
    int order;
    
    size--;
    size >>= PAGE_SHIFT;
#if BITS_PER_LONG == 32
    order = fls(size);
#else
    order = fls64(size);
#endif
    return order;
}

/**
 * get_order - Determine the allocation order of a memory size
 * @size: The size for which to get the order
 *
 * Determine the allocation order of a particular sized block of memory.  This
 * is on a logarithmic scale, where:
 *
 *    0 -> 2^0 * PAGE_SIZE and below
 *    1 -> 2^1 * PAGE_SIZE to 2^0 * PAGE_SIZE + 1
 *    2 -> 2^2 * PAGE_SIZE to 2^1 * PAGE_SIZE + 1
 *    3 -> 2^3 * PAGE_SIZE to 2^2 * PAGE_SIZE + 1
 *    4 -> 2^4 * PAGE_SIZE to 2^3 * PAGE_SIZE + 1
 *    ...
 *
 * The order returned is used to find the smallest allocation granule required
 * to hold an object of the specified size.
 *
 * The result is undefined if the size is 0.
 *
 * This function may be used to initialise variables with compile time
 * evaluations of constants.
 */
#define get_order(n)                        \
(                                \
__builtin_constant_p(n) ? (                \
((n) == 0UL) ? BITS_PER_LONG - PAGE_SHIFT :    \
(((n) < (1UL << PAGE_SHIFT)) ? 0 :        \
ilog2((n) - 1) - PAGE_SHIFT + 1)        \
) :                            \
__get_order(n)                        \
)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static inline void * ERR_PTR(long error)
{
    return (void *) error;
}





#endif /* kernel_h */
