/* SPDX-License-Identifier: GPL-2.0 */

//
//  jiffies.h
//  IntelWifi
//
//  Created by Roman Peshkov on 02/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef utils_h
#define utils_h

// Copy pasted from iwi2200 project

#define HZ 1000
#define MSEC_PER_SEC 100

#define jiffies                         \
({                                      \
    uint64_t m,f;                       \
    clock_get_uptime(&m);               \
    absolutetime_to_nanoseconds(m,&f);  \
    ((f * HZ) / 1000000000);            \
})

inline unsigned long jiffies_to_msecs(const unsigned long j)
{
#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
    return (MSEC_PER_SEC / HZ) * j;
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
    return (j + (HZ / MSEC_PER_SEC) - 1)/(HZ / MSEC_PER_SEC);
#else
    return (j * MSEC_PER_SEC) / HZ;
#endif
}

inline unsigned long msecs_to_jiffies(const unsigned long m)
{
    //if (m > jiffies_to_msecs(MAX_JIFFY_OFFSET)) return MAX_JIFFY_OFFSET;
#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
    return (m + (MSEC_PER_SEC / HZ) - 1) / (MSEC_PER_SEC / HZ);
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
    return m * (HZ / MSEC_PER_SEC);
#else
    return (m * HZ + MSEC_PER_SEC - 1) / MSEC_PER_SEC;
#endif
}

/*
 *    These inlines deal with timer wrapping correctly. You are
 *    strongly encouraged to use them
 *    1. Because people otherwise forget
 *    2. Because if the timer wrap changes in future you won't have to
 *       alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define time_after(a,b) ((long)((b) - (a)) < 0)
#define time_before(a,b) time_after(b,a)

#define time_after_eq(a,b) ((long)((a) - (b)) >= 0)
#define time_before_eq(a,b)    time_after_eq(b,a)


#endif /* utils_h */
