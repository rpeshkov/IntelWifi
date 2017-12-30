//
//  types.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef types_h
#define types_h

#include <IOKit/IOTypes.h>
#include <libkern/OSAtomic.h>

#include "bitfield.h"
#include "kernel.h"

typedef UInt8  u8;
typedef UInt16 u16;
typedef UInt32 u32;
typedef UInt64 u64;

typedef UInt8 __u8;
typedef UInt16 __u16;
typedef UInt32 __u32;
typedef UInt64 __u64;

typedef UInt16 __le16;
typedef UInt32 __le32;
typedef UInt64 __le64;


typedef SInt8  s8;
typedef SInt16 s16;
typedef SInt32 s32;
typedef SInt64 s64;

typedef SInt8  __s8;
typedef SInt16 __s16;
typedef SInt32 __s32;
typedef SInt64 __s64;


//#define __unused __attribute__((__unused__))
#define __packed __attribute__((packed))

typedef u64 dma_addr_t;

/**
 * upper_32_bits - return bits 32-63 of a number
 * @n: the number we're accessing
 *
 * A basic shift-right of a 64- or 32-bit quantity.  Use this to suppress
 * the "right shift count >= width of type" warning when that quantity is
 * 32-bits.
 */
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))

#define __bitwise
#define __force



#define cpu_to_le64 __cpu_to_le64
#define le64_to_cpu __le64_to_cpu
#define cpu_to_le32 __cpu_to_le32
#define le32_to_cpu __le32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define le16_to_cpu __le16_to_cpu
#define cpu_to_be64 __cpu_to_be64
#define be64_to_cpu __be64_to_cpu
#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu
#define cpu_to_be16 __cpu_to_be16
#define be16_to_cpu __be16_to_cpu




#define __cpu_to_le64(x) ((__force __le64)(__u64)(x))
#define __le64_to_cpu(x) ((__force __u64)(__le64)(x))
#define __cpu_to_le32(x) ((__force __le32)(__u32)(x))
#define __le32_to_cpu(x) ((__force __u32)(__le32)(x))
#define __cpu_to_le16(x) ((__force __le16)(__u16)(x))
#define __le16_to_cpu(x) ((__force __u16)(__le16)(x))
#define __cpu_to_be64(x) ((__force __be64)__swab64((x)))
#define __be64_to_cpu(x) __swab64((__force __u64)(__be64)(x))
#define __cpu_to_be32(x) ((__force __be32)__swab32((x)))
#define __be32_to_cpu(x) __swab32((__force __u32)(__be32)(x))
#define __cpu_to_be16(x) ((__force __be16)__swab16((x)))
#define __be16_to_cpu(x) __swab16((__force __u16)(__be16)(x))

#define le32_to_cpup __le32_to_cpup
#define le16_to_cpup __le16_to_cpup


static inline __u32 __le32_to_cpup(const __le32 *p)
{
    return (__force __u32)*p;
}

static inline __u16 __le16_to_cpup(const __le16 *p)
{
    return (__force __u16)*p;
}






static inline int
test_bit(int nr, const volatile unsigned long *addr)
{
    return (OSAddAtomic(0, addr) & (1 << nr)) != 0;
}

static inline void
clear_bit(unsigned int nr, volatile unsigned long *addr)
{
    OSTestAndClear(nr, (volatile UInt8 *)addr);
}

static inline int
test_and_clear_bit(unsigned int nr, volatile unsigned long *addr)
{
    return !OSTestAndClear(nr, (volatile UInt8 *)addr);
}

static inline int
test_and_set_bit(unsigned int nr, volatile unsigned long *addr)
{
    return OSTestAndSet(nr, (volatile UInt8 *)addr);
}

static inline void
set_bit(unsigned int nr, volatile unsigned long *addr)
{
    OSTestAndSet(nr, (volatile UInt8 *)addr);
}

#define __set_bit set_bit
#define __clear_bit clear_bit


#define ETHTOOL_FWVERS_LEN    32
#define ETHTOOL_BUSINFO_LEN    32
#define ETHTOOL_EROMVERS_LEN    32

#define RT_ALIGN_T(u, uAlignment, type) ( ((type)(u) + ((uAlignment) - 1)) & ~(type)((uAlignment) - 1) )
#define RT_ALIGN_Z(cb, uAlignment)              RT_ALIGN_T(cb, uAlignment, size_t)
#define ALIGN RT_ALIGN_Z




#endif /* types_h */
