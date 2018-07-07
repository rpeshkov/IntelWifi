//
//  bitfield.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef bitfield_h
#define bitfield_h

#include <libkern/OSTypes.h>
#include <libkern/OSAtomic.h>

#define BITS_PER_LONG 64


#define BIT(nr)            (1UL << (nr))
#define BIT_ULL(nr)        (1ULL << (nr))
#define BIT_MASK(nr)        (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)    (1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)    ((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE        8
#define BITS_TO_LONGS(nr)    DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))



static inline UInt64 OSBitwiseAtomic64(unsigned long and_mask, unsigned long or_mask, unsigned long xor_mask, unsigned long * value)
{
    unsigned long    oldValue;
    unsigned long    newValue;

    do {
        oldValue = *value;
        newValue = ((oldValue & and_mask) | or_mask) ^ xor_mask;
    } while (! OSCompareAndSwap64(oldValue, newValue, value));
    
    return oldValue;
}

static inline unsigned long OSBitAndAtomic64(unsigned long mask, unsigned long * value)
{
    return OSBitwiseAtomic64(mask, 0, 0, value);
}

static inline unsigned long OSBitOrAtomic64(unsigned long mask, unsigned long * value)
{
    return OSBitwiseAtomic64(-1, mask, 0, value);
}

//static Boolean OSTestAndSetClear(UInt32 bit, Boolean wantSet, UInt8 * startAddress)
//{
//    UInt8        mask = 1;
//    UInt8        oldValue;
//    UInt8        wantValue;
//
//    startAddress += (bit / 8);
//    mask <<= (7 - (bit % 8));
//    wantValue = wantSet ? mask : 0;
//
//    do {
//        oldValue = *startAddress;
//        if ((oldValue & mask) == wantValue) {
//            break;
//        }
//    } while (! OSCompareAndSwap8(oldValue, (oldValue & ~mask) | wantValue, startAddress));
//
//    return (oldValue & mask) == wantValue;
//}

/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 *
 * Note: there are no guarantees that this function will not be reordered
 * on non x86 architectures, so if you are writing portable code,
 * make sure not to rely on its reordering guarantees.
 *
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline void set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    OSBitOrAtomic64(mask, p);
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_atomic() and/or smp_mb__after_atomic()
 * in order to ensure changes are visible on other processors.
 */
static inline void clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    OSBitAndAtomic64(~mask, p);
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It may be reordered on other architectures than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long old;
//    unsigned long flags;
    
    old = *p;
    *p = old | mask;
    
    return (old & mask) != 0;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It can be reorderdered on other architectures other than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long old;
//    unsigned long flags;
    
    old = *p;
    *p = old & ~mask;
    
    return (old & mask) != 0;
}






static inline int
test_bit(int nr, const volatile unsigned long *addr)
{
    return (OSAddAtomic(0, addr) & (1 << nr)) != 0;
}



#define __bf_shf(x) (__builtin_ffsll(x) - 1)


/**
 * FIELD_PREP() - prepare a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_val:  value to put in the field
 *
 * FIELD_PREP() masks and shifts up the value.  The result should
 * be combined with other fields of the bitfield using logical OR.
 */
#define FIELD_PREP(_mask, _val)                        \
({                                \
((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask);    \
})


/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */

static inline int linux_fls(int x)
{
    int r = 32;
    
    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

unsigned long find_next_bit(const unsigned long *addr, unsigned long size, unsigned long offset);

#define for_each_set_bit(bit, addr, size) \
        for ((bit) = find_first_bit((addr), (size));        \
            (bit) < (size);                    \
            (bit) = find_next_bit((addr), (size), (bit) + 1))

#define find_first_bit(addr, size) find_next_bit((addr), (size), 0)




#endif /* bitfield_h */
