/******************************************************************************
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2008 - 2014 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 * Contact Information:
 *  Intel Linux Wireless <linuxwifi@intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 *****************************************************************************/

//
//  IwlDvmOpMode_lib.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 18/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

extern "C" {
#include "iwl-trans.h"
#include "agn.h"
    
}


#include "IwlDvmOpMode.hpp"


// line 49
int iwlagn_send_tx_power(struct iwl_priv *priv)
{
    struct iwlagn_tx_power_dbm_cmd tx_power_cmd;
    u8 tx_ant_cfg_cmd;
    
    if (test_bit(STATUS_SCAN_HW, &priv->status))
        return -EAGAIN;
    
    /* half dBm need to multiply */
    tx_power_cmd.global_lmt = (s8)(2 * priv->tx_power_user_lmt);
    
    if (tx_power_cmd.global_lmt > priv->nvm_data->max_tx_pwr_half_dbm) {
        /*
         * For the newer devices which using enhanced/extend tx power
         * table in EEPROM, the format is in half dBm. driver need to
         * convert to dBm format before report to mac80211.
         * By doing so, there is a possibility of 1/2 dBm resolution
         * lost. driver will perform "round-up" operation before
         * reporting, but it will cause 1/2 dBm tx power over the
         * regulatory limit. Perform the checking here, if the
         * "tx_power_user_lmt" is higher than EEPROM value (in
         * half-dBm format), lower the tx power based on EEPROM
         */
        tx_power_cmd.global_lmt =
        priv->nvm_data->max_tx_pwr_half_dbm;
    }
    tx_power_cmd.flags = IWLAGN_TX_POWER_NO_CLOSED;
    tx_power_cmd.srv_chan_lmt = IWLAGN_TX_POWER_AUTO;
    
    if (IWL_UCODE_API(priv->fw->ucode_ver) == 1)
        tx_ant_cfg_cmd = REPLY_TX_POWER_DBM_CMD_V1;
    else
        tx_ant_cfg_cmd = REPLY_TX_POWER_DBM_CMD;
    
    return iwl_dvm_send_cmd_pdu(priv, tx_ant_cfg_cmd, 0,
                                sizeof(tx_power_cmd), &tx_power_cmd);
}

int iwlagn_hwrate_to_mac80211_idx(u32 rate_n_flags, enum nl80211_band band)
{
    int idx = 0;
    int band_offset = 0;
    
    /* HT rate format: mac80211 wants an MCS number, which is just LSB */
    if (rate_n_flags & RATE_MCS_HT_MSK) {
        idx = (rate_n_flags & 0xff);
        return idx;
        /* Legacy rate format, search for match in table */
    } else {
        if (band == NL80211_BAND_5GHZ)
            band_offset = IWL_FIRST_OFDM_RATE;
//        for (idx = band_offset; idx < IWL_RATE_COUNT_LEGACY; idx++)
//            if (iwl_rates[idx].plcp == (rate_n_flags & 0xFF))
//                return idx - band_offset;
    }
    
    return -1;
}


/*
 * BT coex
 */
/* Notmal TDM */
static const __le32 iwlagn_def_3w_lookup[IWLAGN_BT_DECISION_LUT_SIZE] = {
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaeaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xcc00ff28),
    cpu_to_le32(0x0000aaaa),
    cpu_to_le32(0xcc00aaaa),
    cpu_to_le32(0x0000aaaa),
    cpu_to_le32(0xc0004000),
    cpu_to_le32(0x00004000),
    cpu_to_le32(0xf0005000),
    cpu_to_le32(0xf0005000),
};

/* Full concurrency */
static const __le32 iwlagn_concurrent_lookup[IWLAGN_BT_DECISION_LUT_SIZE] = {
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0xaaaaaaaa),
    cpu_to_le32(0x00000000),
    cpu_to_le32(0x00000000),
    cpu_to_le32(0x00000000),
    cpu_to_le32(0x00000000),
};


// line 224
void iwlagn_send_advance_bt_config(struct iwl_priv *priv)
{
    struct iwl_basic_bt_cmd basic = {
        .max_kill = IWLAGN_BT_MAX_KILL_DEFAULT,
        .bt3_timer_t7_value = IWLAGN_BT3_T7_DEFAULT,
        .bt3_prio_sample_time = IWLAGN_BT3_PRIO_SAMPLE_DEFAULT,
        .bt3_timer_t2_value = IWLAGN_BT3_T2_DEFAULT,
    };
    struct iwl_bt_cmd_v1 bt_cmd_v1;
    struct iwl_bt_cmd_v2 bt_cmd_v2;
    int ret;
    
    BUILD_BUG_ON(sizeof(iwlagn_def_3w_lookup) != sizeof(basic.bt3_lookup_table));
    
    if (priv->lib->bt_params) {
        /*
         * newer generation of devices (2000 series and newer)
         * use the version 2 of the bt command
         * we need to make sure sending the host command
         * with correct data structure to avoid uCode assert
         */
        if (priv->lib->bt_params->bt_session_2) {
            bt_cmd_v2.prio_boost = cpu_to_le32(
                                               priv->lib->bt_params->bt_prio_boost);
            bt_cmd_v2.tx_prio_boost = 0;
            bt_cmd_v2.rx_prio_boost = 0;
        } else {
            /* older version only has 8 bits */
            //WARN_ON(priv->lib->bt_params->bt_prio_boost & ~0xFF);
            bt_cmd_v1.prio_boost =
            priv->lib->bt_params->bt_prio_boost;
            bt_cmd_v1.tx_prio_boost = 0;
            bt_cmd_v1.rx_prio_boost = 0;
        }
    } else {
        IWL_ERR(priv, "failed to construct BT Coex Config\n");
        return;
    }
    
    /*
     * Possible situations when BT needs to take over for receive,
     * at the same time where STA needs to response to AP's frame(s),
     * reduce the tx power of the required response frames, by that,
     * allow the concurrent BT receive & WiFi transmit
     * (BT - ANT A, WiFi -ANT B), without interference to one another
     *
     * Reduced tx power apply to control frames only (ACK/Back/CTS)
     * when indicated by the BT config command
     */
    basic.kill_ack_mask = priv->kill_ack_mask;
    basic.kill_cts_mask = priv->kill_cts_mask;
    if (priv->reduced_txpower)
        basic.reduce_txpower = IWLAGN_BT_REDUCED_TX_PWR;
    basic.valid = priv->bt_valid;
    
    /*
     * Configure BT coex mode to "no coexistence" when the
     * user disabled BT coexistence, we have no interface
     * (might be in monitor mode), or the interface is in
     * IBSS mode (no proper uCode support for coex then).
     */
    if (!iwlwifi_mod_params.bt_coex_active ||
        priv->iw_mode == NL80211_IFTYPE_ADHOC) {
        basic.flags = IWLAGN_BT_FLAG_COEX_MODE_DISABLED;
    } else {
        basic.flags = IWLAGN_BT_FLAG_COEX_MODE_3W <<
        IWLAGN_BT_FLAG_COEX_MODE_SHIFT;
        
        if (!priv->bt_enable_pspoll)
            basic.flags |= IWLAGN_BT_FLAG_SYNC_2_BT_DISABLE;
        else
            basic.flags &= ~IWLAGN_BT_FLAG_SYNC_2_BT_DISABLE;
        
        if (priv->bt_ch_announce)
            basic.flags |= IWLAGN_BT_FLAG_CHANNEL_INHIBITION;
        IWL_DEBUG_COEX(priv, "BT coex flag: 0X%x\n", basic.flags);
    }
    priv->bt_enable_flag = basic.flags;
    if (priv->bt_full_concurrent)
        memcpy(basic.bt3_lookup_table, iwlagn_concurrent_lookup,
               sizeof(iwlagn_concurrent_lookup));
    else
        memcpy(basic.bt3_lookup_table, iwlagn_def_3w_lookup,
               sizeof(iwlagn_def_3w_lookup));
    
    IWL_DEBUG_COEX(priv, "BT coex %s in %s mode\n",
                   basic.flags ? "active" : "disabled",
                   priv->bt_full_concurrent ?
                   "full concurrency" : "3-wire");
    
    if (priv->lib->bt_params->bt_session_2) {
        memcpy(&bt_cmd_v2.basic, &basic,
               sizeof(basic));
        ret = iwl_dvm_send_cmd_pdu(priv, REPLY_BT_CONFIG,
                                   0, sizeof(bt_cmd_v2), &bt_cmd_v2);
    } else {
        memcpy(&bt_cmd_v1.basic, &basic,
               sizeof(basic));
        ret = iwl_dvm_send_cmd_pdu(priv, REPLY_BT_CONFIG,
                                   0, sizeof(bt_cmd_v1), &bt_cmd_v1);
    }
    if (ret)
        IWL_ERR(priv, "failed to send BT Coex Config\n");
    
}



// line 718
static bool is_single_rx_stream(struct iwl_priv *priv)
{
    return priv->current_ht_config.smps == IEEE80211_SMPS_STATIC ||
    priv->current_ht_config.single_chain_sufficient;
}


#define IWL_NUM_RX_CHAINS_MULTIPLE    3
#define IWL_NUM_RX_CHAINS_SINGLE    2
#define IWL_NUM_IDLE_CHAINS_DUAL    2
#define IWL_NUM_IDLE_CHAINS_SINGLE    1


/* line 729
 * Determine how many receiver/antenna chains to use.
 *
 * More provides better reception via diversity.  Fewer saves power
 * at the expense of throughput, but only when not in powersave to
 * start with.
 *
 * MIMO (dual stream) requires at least 2, but works better with 3.
 * This does not determine *which* chains to use, just how many.
 */
static int iwl_get_active_rx_chain_count(struct iwl_priv *priv)
{
    if (priv->lib->bt_params &&
        priv->lib->bt_params->advanced_bt_coexist &&
        (priv->bt_full_concurrent ||
         priv->bt_traffic_load >= IWL_BT_COEX_TRAFFIC_LOAD_HIGH)) {
            /*
             * only use chain 'A' in bt high traffic load or
             * full concurrency mode
             */
            return IWL_NUM_RX_CHAINS_SINGLE;
        }
    /* # of Rx chains to use when expecting MIMO. */
    if (is_single_rx_stream(priv))
        return IWL_NUM_RX_CHAINS_SINGLE;
    else
        return IWL_NUM_RX_CHAINS_MULTIPLE;
}

/* line 758
 * When we are in power saving mode, unless device support spatial
 * multiplexing power save, use the active count for rx chain count.
 */
static int iwl_get_idle_rx_chain_count(struct iwl_priv *priv, int active_cnt)
{
    /* # Rx chains when idling, depending on SMPS mode */
    switch (priv->current_ht_config.smps) {
        case IEEE80211_SMPS_STATIC:
        case IEEE80211_SMPS_DYNAMIC:
            return IWL_NUM_IDLE_CHAINS_SINGLE;
        case IEEE80211_SMPS_AUTOMATIC:
        case IEEE80211_SMPS_OFF:
            return active_cnt;
        default:
            IWL_DEBUG_RX(priv, "invalid SMPS mode %d", priv->current_ht_config.smps);
            return active_cnt;
    }
}

// line 779
/* up to 4 chains */
static u8 iwl_count_chain_bitmap(u32 chain_bitmap)
{
    u8 res;
    res = (chain_bitmap & BIT(0)) >> 0;
    res += (chain_bitmap & BIT(1)) >> 1;
    res += (chain_bitmap & BIT(2)) >> 2;
    res += (chain_bitmap & BIT(3)) >> 3;
    return res;
}


/** line 790
 * iwlagn_set_rxon_chain - Set up Rx chain usage in "staging" RXON image
 *
 * Selects how many and which Rx receivers/antennas/chains to use.
 * This should not be used for scan command ... it puts data in wrong place.
 */
void iwlagn_set_rxon_chain(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    bool is_single = is_single_rx_stream(priv);
    bool is_cam = !test_bit(STATUS_POWER_PMI, &priv->status);
    u8 idle_rx_cnt, active_rx_cnt, valid_rx_cnt;
    u32 active_chains;
    u16 rx_chain;
    
    /* Tell uCode which antennas are actually connected.
     * Before first association, we assume all antennas are connected.
     * Just after first association, iwl_chain_noise_calibration()
     *    checks which antennas actually *are* connected. */
    if (priv->chain_noise_data.active_chains)
        active_chains = priv->chain_noise_data.active_chains;
    else
        active_chains = priv->nvm_data->valid_rx_ant;
    
    if (priv->lib->bt_params && priv->lib->bt_params->advanced_bt_coexist
    && (priv->bt_full_concurrent || priv->bt_traffic_load >= IWL_BT_COEX_TRAFFIC_LOAD_HIGH)) {
        /*
         * only use chain 'A' in bt high traffic load or
         * full concurrency mode
         */
        active_chains = first_antenna(active_chains);
    }
    
    rx_chain = active_chains << RXON_RX_CHAIN_VALID_POS;
    
    /* How many receivers should we use? */
    active_rx_cnt = iwl_get_active_rx_chain_count(priv);
    idle_rx_cnt = iwl_get_idle_rx_chain_count(priv, active_rx_cnt);
    
    
    /* correct rx chain count according hw settings
     * and chain noise calibration
     */
    valid_rx_cnt = iwl_count_chain_bitmap(active_chains);
    if (valid_rx_cnt < active_rx_cnt)
        active_rx_cnt = valid_rx_cnt;
    
    if (valid_rx_cnt < idle_rx_cnt)
        idle_rx_cnt = valid_rx_cnt;
    
    rx_chain |= active_rx_cnt << RXON_RX_CHAIN_MIMO_CNT_POS;
    rx_chain |= idle_rx_cnt  << RXON_RX_CHAIN_CNT_POS;
    
    ctx->staging.rx_chain = cpu_to_le16(rx_chain);
    
    if (!is_single && (active_rx_cnt >= IWL_NUM_RX_CHAINS_SINGLE) && is_cam)
        ctx->staging.rx_chain |= RXON_RX_CHAIN_MIMO_FORCE_MSK;
    else
        ctx->staging.rx_chain &= ~RXON_RX_CHAIN_MIMO_FORCE_MSK;
    
    IWL_DEBUG_ASSOC(priv, "rx_chain=0x%X active=%d idle=%d\n", ctx->staging.rx_chain, active_rx_cnt, idle_rx_cnt);
    
//    WARN_ON(active_rx_cnt == 0 || idle_rx_cnt == 0 ||
//            active_rx_cnt < idle_rx_cnt);
}

// line 859
u8 iwl_toggle_tx_ant(struct iwl_priv *priv, u8 ant, u8 valid)
{
    int i;
    u8 ind = ant;
    
    if (priv->band == NL80211_BAND_2GHZ && priv->bt_traffic_load >= IWL_BT_COEX_TRAFFIC_LOAD_HIGH)
        return 0;
    
    for (i = 0; i < RATE_ANT_NUM - 1; i++) {
        ind = (ind + 1) < RATE_ANT_NUM ?  ind + 1 : 0;
        if (valid & BIT(ind))
            return ind;
    }
    return ant;
}



// line 1237
int iwl_dvm_send_cmd(struct iwl_priv *priv, struct iwl_host_cmd *cmd)
{
    if (iwl_is_rfkill(priv) || iwl_is_ctkill(priv)) {
        IWL_WARN(priv, "Not sending command - %s KILL\n", iwl_is_rfkill(priv) ? "RF" : "CT");
        return -EIO;
    }
    
    if (test_bit(STATUS_FW_ERROR, &priv->status)) {
        const char* str = iwl_get_cmd_string(priv->trans, cmd->id);
        IWL_ERR(priv, "Command %s failed: FW Error\n", str);
        return -EIO;
    }
    
    /*
     * This can happen upon FW ASSERT: we clear the STATUS_FW_ERROR flag
     * in iwl_down but cancel the workers only later.
     */
    if (!priv->ucode_loaded) {
        IWL_ERR(priv, "Fw not loaded - dropping CMD: %x\n", cmd->id);
        return -EIO;
    }
    
    /*
     * Synchronous commands from this op-mode must hold
     * the mutex, this ensures we don't try to send two
     * (or more) synchronous commands at a time.
     */
    // TODO: Implement
//    if (!(cmd->flags & CMD_ASYNC))
//        lockdep_assert_held(&priv->mutex);
    
    return iwl_trans_send_cmd(priv->trans, cmd);
}

// line 1271
int iwl_dvm_send_cmd_pdu(struct iwl_priv *priv, u8 id, u32 flags, u16 len, const void *data)
{
    struct iwl_host_cmd cmd = {
        .id = id,
        .len = { len, },
        .data = { data, },
        .flags = flags,
    };
    
    return iwl_dvm_send_cmd(priv, &cmd);
}
