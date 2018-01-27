//
//  bitfield.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef bitfield_h
#define bitfield_h

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

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define for_each_set_bit(bit, addr, size) \
for ((bit) = find_first_bit((addr), (size));        \
(bit) < (size);                    \
(bit) = find_next_bit((addr), (size), (bit) + 1))



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

static inline int fls(int x)
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
