/******************************************************************************
 *
 * Copyright(c) 2005 - 2014 Intel Corporation. All rights reserved.
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

#include "dev.h"
#include "agn.h"

#include "IwlDvmOpMode.hpp"

#define RS_NAME "iwl-agn-rs"

#define NUM_TRY_BEFORE_ANT_TOGGLE 1
#define IWL_NUMBER_TRY      1
#define IWL_HT_NUMBER_TRY   3

#define IWL_RATE_MAX_WINDOW        62    /* # tx in history window */
#define IWL_RATE_MIN_FAILURE_TH        6    /* min failures to calc tpt */
#define IWL_RATE_MIN_SUCCESS_TH        8    /* min successes to calc tpt */

/* max allowed rate miss before sync LQ cmd */
#define IWL_MISSED_RATE_MAX        15
/* max time to accum history 2 seconds */
#define IWL_RATE_SCALE_FLUSH_INTVL   (3*HZ)

static u8 rs_ht_to_legacy[] = {
    IWL_RATE_6M_INDEX, IWL_RATE_6M_INDEX,
    IWL_RATE_6M_INDEX, IWL_RATE_6M_INDEX,
    IWL_RATE_6M_INDEX,
    IWL_RATE_6M_INDEX, IWL_RATE_9M_INDEX,
    IWL_RATE_12M_INDEX, IWL_RATE_18M_INDEX,
    IWL_RATE_24M_INDEX, IWL_RATE_36M_INDEX,
    IWL_RATE_48M_INDEX, IWL_RATE_54M_INDEX
};

static const u8 ant_toggle_lookup[] = {
    /*ANT_NONE -> */ ANT_NONE,
    /*ANT_A    -> */ ANT_B,
    /*ANT_B    -> */ ANT_C,
    /*ANT_AB   -> */ ANT_BC,
    /*ANT_C    -> */ ANT_A,
    /*ANT_AC   -> */ ANT_AB,
    /*ANT_BC   -> */ ANT_AC,
    /*ANT_ABC  -> */ ANT_ABC,
};

#define IWL_DECLARE_RATE_INFO(r, s, ip, in, rp, rn, pp, np)    \
[IWL_RATE_##r##M_INDEX] = { IWL_RATE_##r##M_PLCP,      \
IWL_RATE_SISO_##s##M_PLCP, \
IWL_RATE_MIMO2_##s##M_PLCP,\
IWL_RATE_MIMO3_##s##M_PLCP,\
IWL_RATE_##r##M_IEEE,      \
IWL_RATE_##ip##M_INDEX,    \
IWL_RATE_##in##M_INDEX,    \
IWL_RATE_##rp##M_INDEX,    \
IWL_RATE_##rn##M_INDEX,    \
IWL_RATE_##pp##M_INDEX,    \
IWL_RATE_##np##M_INDEX }

/*
 * Parameter order:
 *   rate, ht rate, prev rate, next rate, prev tgg rate, next tgg rate
 *
 * If there isn't a valid next or previous rate then INV is used which
 * maps to IWL_RATE_INVALID
 *
 */
const struct iwl_rate_info iwl_rates[IWL_RATE_COUNT] = {
    IWL_DECLARE_RATE_INFO(1, INV, INV, 2, INV, 2, INV, 2),    /*  1mbps */
    IWL_DECLARE_RATE_INFO(2, INV, 1, 5, 1, 5, 1, 5),          /*  2mbps */
    IWL_DECLARE_RATE_INFO(5, INV, 2, 6, 2, 11, 2, 11),        /*5.5mbps */
    IWL_DECLARE_RATE_INFO(11, INV, 9, 12, 9, 12, 5, 18),      /* 11mbps */
    IWL_DECLARE_RATE_INFO(6, 6, 5, 9, 5, 11, 5, 11),        /*  6mbps */
    IWL_DECLARE_RATE_INFO(9, 6, 6, 11, 6, 11, 5, 11),       /*  9mbps */
    IWL_DECLARE_RATE_INFO(12, 12, 11, 18, 11, 18, 11, 18),   /* 12mbps */
    IWL_DECLARE_RATE_INFO(18, 18, 12, 24, 12, 24, 11, 24),   /* 18mbps */
    IWL_DECLARE_RATE_INFO(24, 24, 18, 36, 18, 36, 18, 36),   /* 24mbps */
    IWL_DECLARE_RATE_INFO(36, 36, 24, 48, 24, 48, 24, 48),   /* 36mbps */
    IWL_DECLARE_RATE_INFO(48, 48, 36, 54, 36, 54, 36, 54),   /* 48mbps */
    IWL_DECLARE_RATE_INFO(54, 54, 48, INV, 48, INV, 48, INV),/* 54mbps */
    IWL_DECLARE_RATE_INFO(60, 60, 48, INV, 48, INV, 48, INV),/* 60mbps */
    /* FIXME:RS:          ^^    should be INV (legacy) */
};

