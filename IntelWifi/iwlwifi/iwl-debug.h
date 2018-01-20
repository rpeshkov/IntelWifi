/******************************************************************************
 *
 * Copyright(c) 2003 - 2014 Intel Corporation. All rights reserved.
 *
 * Portions of this file are derived from the ipw3945 project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 *  Intel Linux Wireless <linuxwifi@intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 *****************************************************************************/

//
//  iwl-debug.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef iwl_debug_h
#define iwl_debug_h

#import "../Logging.h"

#include "iwl-modparams.h"


static inline bool iwl_have_debug_level(u32 level)
{
//#ifdef CONFIG_IWLWIFI_DEBUG
    return iwlwifi_mod_params.debug_level & level;
//#else
//    return false;
//#endif
}

/* 0x0000000F - 0x00000001 */
#define IWL_DL_INFO        0x00000001
#define IWL_DL_MAC80211        0x00000002
#define IWL_DL_HCMD        0x00000004
#define IWL_DL_TDLS        0x00000008
/* 0x000000F0 - 0x00000010 */
#define IWL_DL_QUOTA        0x00000010
#define IWL_DL_TE        0x00000020
#define IWL_DL_EEPROM        0x00000040
#define IWL_DL_RADIO        0x00000080
/* 0x00000F00 - 0x00000100 */
#define IWL_DL_POWER        0x00000100
#define IWL_DL_TEMP        0x00000200
#define IWL_DL_RPM        0x00000400
#define IWL_DL_SCAN        0x00000800
/* 0x0000F000 - 0x00001000 */
#define IWL_DL_ASSOC        0x00001000
#define IWL_DL_DROP        0x00002000
#define IWL_DL_LAR        0x00004000
#define IWL_DL_COEX        0x00008000
/* 0x000F0000 - 0x00010000 */
#define IWL_DL_FW        0x00010000
#define IWL_DL_RF_KILL        0x00020000
#define IWL_DL_FW_ERRORS    0x00040000
/* 0x00F00000 - 0x00100000 */
#define IWL_DL_RATE        0x00100000
#define IWL_DL_CALIB        0x00200000
#define IWL_DL_WEP        0x00400000
#define IWL_DL_TX        0x00800000
/* 0x0F000000 - 0x01000000 */
#define IWL_DL_RX        0x01000000
#define IWL_DL_ISR        0x02000000
#define IWL_DL_HT        0x04000000
#define IWL_DL_EXTERNAL        0x08000000
/* 0xF0000000 - 0x10000000 */
#define IWL_DL_11H        0x10000000
#define IWL_DL_STATS        0x20000000
#define IWL_DL_TX_REPLY        0x40000000
#define IWL_DL_TX_QUEUES    0x80000000



#endif /* iwl_debug_h */
