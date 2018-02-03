/*
 * INET        An implementation of the TCP/IP protocol suite for the LINUX
 *        operating system.  NET  is implemented using the  BSD Socket
 *        interface as the means of communication with the user level.
 *
 *        Definitions for the Ethernet handlers.
 *
 * Version:    @(#)eth.h    1.0.4    05/13/93
 *
 * Authors:    Ross Biro
 *        Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *        Relocated to include/linux where it belongs by Alan Cox
 *                            <gw4pts@gw4pts.ampr.org>
 *
 *        This program is free software; you can redistribute it and/or
 *        modify it under the terms of the GNU General Public License
 *        as published by the Free Software Foundation; either version
 *        2 of the License, or (at your option) any later version.
 *
 */

//
//  etherdevice.h
//  IntelWifi
//
//  Created by Roman Peshkov on 18/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef etherdevice_h
#define etherdevice_h

#include <linux/types.h>

/** line 157
 * is_broadcast_ether_addr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is the broadcast address.
 *
 * Please note: addr must be aligned to u16.
 */
static inline bool is_broadcast_ether_addr(const u8 *addr)
{
    return (*(const u16 *)(addr + 0) &
            *(const u16 *)(addr + 2) &
            *(const u16 *)(addr + 4)) == 0xffff;
}

/** line 235
 * eth_broadcast_addr - Assign broadcast address
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Assign the broadcast address to the given address array.
 */
static inline void eth_broadcast_addr(u8 *addr)
{
    memset(addr, 0xff, ETH_ALEN);
}


/** line 309
 * ether_addr_equal - Compare two Ethernet addresses
 * @addr1: Pointer to a six-byte array containing the Ethernet address
 * @addr2: Pointer other six-byte array containing the Ethernet address
 *
 * Compare two Ethernet addresses, returns true if equal
 *
 * Please note: addr1 & addr2 must both be aligned to u16.
 */
static inline bool ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
    u32 fold = ((*(const u32 *)addr1) ^ (*(const u32 *)addr2)) |
    ((*(const u16 *)(addr1 + 4)) ^ (*(const u16 *)(addr2 + 4)));
    
    return fold == 0;
#else
    const u16 *a = (const u16 *)addr1;
    const u16 *b = (const u16 *)addr2;
    
    return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
#endif
}



#endif /* etherdevice_h */
