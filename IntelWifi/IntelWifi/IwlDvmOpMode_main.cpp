/******************************************************************************
 *
 * Copyright(c) 2003 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2015 Intel Deutschland GmbH
 *
 * Portions of this file are derived from the ipw3945 project, as well
 * as portions of the ieee80211 subsystem header files.
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
//  IwlDvmOpMode.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

extern "C" {
#include "iwlwifi/dvm/commands.h"
#include "iwlwifi/dvm/dev.h"
#include "iwlwifi/dvm/agn.h"
#include "iwlwifi/iwl-drv.h"
#include "iwlwifi/iwl-prph.h"
#include "iwlwifi/iwl-io.h"
#include "iwlwifi/iwl-eeprom-read.h"
#include "iwlwifi/iwl-eeprom-parse.h"
#include "tt.h"
}

#include "IwlDvmOpMode.hpp"

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search.
 * A warning will be triggered on violation.
 */
static const struct iwl_hcmd_names iwl_dvm_cmd_names[] = {
    HCMD_NAME(REPLY_ALIVE),
    HCMD_NAME(REPLY_ERROR),
    HCMD_NAME(REPLY_ECHO),
    HCMD_NAME(REPLY_RXON),
    HCMD_NAME(REPLY_RXON_ASSOC),
    HCMD_NAME(REPLY_QOS_PARAM),
    HCMD_NAME(REPLY_RXON_TIMING),
    HCMD_NAME(REPLY_ADD_STA),
    HCMD_NAME(REPLY_REMOVE_STA),
    HCMD_NAME(REPLY_REMOVE_ALL_STA),
    HCMD_NAME(REPLY_TX),
    HCMD_NAME(REPLY_TXFIFO_FLUSH),
    HCMD_NAME(REPLY_WEPKEY),
    HCMD_NAME(REPLY_LEDS_CMD),
    HCMD_NAME(REPLY_TX_LINK_QUALITY_CMD),
    HCMD_NAME(COEX_PRIORITY_TABLE_CMD),
    HCMD_NAME(COEX_MEDIUM_NOTIFICATION),
    HCMD_NAME(COEX_EVENT_CMD),
    HCMD_NAME(TEMPERATURE_NOTIFICATION),
    HCMD_NAME(CALIBRATION_CFG_CMD),
    HCMD_NAME(CALIBRATION_RES_NOTIFICATION),
    HCMD_NAME(CALIBRATION_COMPLETE_NOTIFICATION),
    HCMD_NAME(REPLY_QUIET_CMD),
    HCMD_NAME(REPLY_CHANNEL_SWITCH),
    HCMD_NAME(CHANNEL_SWITCH_NOTIFICATION),
    HCMD_NAME(REPLY_SPECTRUM_MEASUREMENT_CMD),
    HCMD_NAME(SPECTRUM_MEASURE_NOTIFICATION),
    HCMD_NAME(POWER_TABLE_CMD),
    HCMD_NAME(PM_SLEEP_NOTIFICATION),
    HCMD_NAME(PM_DEBUG_STATISTIC_NOTIFIC),
    HCMD_NAME(REPLY_SCAN_CMD),
    HCMD_NAME(REPLY_SCAN_ABORT_CMD),
    HCMD_NAME(SCAN_START_NOTIFICATION),
    HCMD_NAME(SCAN_RESULTS_NOTIFICATION),
    HCMD_NAME(SCAN_COMPLETE_NOTIFICATION),
    HCMD_NAME(BEACON_NOTIFICATION),
    HCMD_NAME(REPLY_TX_BEACON),
    HCMD_NAME(WHO_IS_AWAKE_NOTIFICATION),
    HCMD_NAME(REPLY_TX_POWER_DBM_CMD),
    HCMD_NAME(QUIET_NOTIFICATION),
    HCMD_NAME(REPLY_TX_PWR_TABLE_CMD),
    HCMD_NAME(REPLY_TX_POWER_DBM_CMD_V1),
    HCMD_NAME(TX_ANT_CONFIGURATION_CMD),
    HCMD_NAME(MEASURE_ABORT_NOTIFICATION),
    HCMD_NAME(REPLY_BT_CONFIG),
    HCMD_NAME(REPLY_STATISTICS_CMD),
    HCMD_NAME(STATISTICS_NOTIFICATION),
    HCMD_NAME(REPLY_CARD_STATE_CMD),
    HCMD_NAME(CARD_STATE_NOTIFICATION),
    HCMD_NAME(MISSED_BEACONS_NOTIFICATION),
    HCMD_NAME(REPLY_CT_KILL_CONFIG_CMD),
    HCMD_NAME(SENSITIVITY_CMD),
    HCMD_NAME(REPLY_PHY_CALIBRATION_CMD),
    HCMD_NAME(REPLY_WIPAN_PARAMS),
    HCMD_NAME(REPLY_WIPAN_RXON),
    HCMD_NAME(REPLY_WIPAN_RXON_TIMING),
    HCMD_NAME(REPLY_WIPAN_RXON_ASSOC),
    HCMD_NAME(REPLY_WIPAN_QOS_PARAM),
    HCMD_NAME(REPLY_WIPAN_WEPKEY),
    HCMD_NAME(REPLY_WIPAN_P2P_CHANNEL_SWITCH),
    HCMD_NAME(REPLY_WIPAN_NOA_NOTIFICATION),
    HCMD_NAME(REPLY_WIPAN_DEACTIVATION_COMPLETE),
    HCMD_NAME(REPLY_RX_PHY_CMD),
    HCMD_NAME(REPLY_RX_MPDU_CMD),
    HCMD_NAME(REPLY_RX),
    HCMD_NAME(REPLY_COMPRESSED_BA),
    HCMD_NAME(REPLY_BT_COEX_PRIO_TABLE),
    HCMD_NAME(REPLY_BT_COEX_PROT_ENV),
    HCMD_NAME(REPLY_BT_COEX_PROFILE_NOTIF),
    HCMD_NAME(REPLY_D3_CONFIG),
    HCMD_NAME(REPLY_WOWLAN_PATTERNS),
    HCMD_NAME(REPLY_WOWLAN_WAKEUP_FILTER),
    HCMD_NAME(REPLY_WOWLAN_TSC_RSC_PARAMS),
    HCMD_NAME(REPLY_WOWLAN_TKIP_PARAMS),
    HCMD_NAME(REPLY_WOWLAN_KEK_KCK_MATERIAL),
    HCMD_NAME(REPLY_WOWLAN_GET_STATUS),
};

static const struct iwl_hcmd_arr iwl_dvm_groups[] = {
    [0x0] = HCMD_ARR(iwl_dvm_cmd_names),
};

//static const struct iwl_op_mode_ops iwl_dvm_ops;


// line 162
void iwl_update_chain_flags(struct iwl_priv *priv)
{
    struct iwl_rxon_context *ctx;
    
    for_each_context(priv, ctx) {
        iwlagn_set_rxon_chain(priv, ctx);
        if (ctx->active.rx_chain != ctx->staging.rx_chain)
            iwlagn_commit_rxon(priv, ctx);
    }
}


// line 374
int iwl_send_statistics_request(struct iwl_priv *priv, u8 flags, bool clear)
{
    struct iwl_statistics_cmd statistics_cmd = {
        .configuration_flags = clear ? IWL_STATS_CONF_CLEAR_STATS : 0,
    };
    
    if (flags & CMD_ASYNC)
        return iwl_dvm_send_cmd_pdu(priv, REPLY_STATISTICS_CMD, CMD_ASYNC, sizeof(struct iwl_statistics_cmd),
                                    &statistics_cmd);
    else
        return iwl_dvm_send_cmd_pdu(priv, REPLY_STATISTICS_CMD, 0, sizeof(struct iwl_statistics_cmd), &statistics_cmd);
}



/* line 590
 * queue/FIFO/AC mapping definitions
 */

static const u8 iwlagn_bss_ac_to_fifo[] = {
    IWL_TX_FIFO_VO,
    IWL_TX_FIFO_VI,
    IWL_TX_FIFO_BE,
    IWL_TX_FIFO_BK,
};

static const u8 iwlagn_bss_ac_to_queue[] = {
    0, 1, 2, 3,
};

static const u8 iwlagn_pan_ac_to_fifo[] = {
    IWL_TX_FIFO_VO_IPAN,
    IWL_TX_FIFO_VI_IPAN,
    IWL_TX_FIFO_BE_IPAN,
    IWL_TX_FIFO_BK_IPAN,
};

static const u8 iwlagn_pan_ac_to_queue[] = {
    7, 6, 5, 4,
};


// line 616
static void iwl_init_context(struct iwl_priv *priv, u32 ucode_flags)
{
    int i;
    
    /*
     * The default context is always valid,
     * the PAN context depends on uCode.
     */
    priv->valid_contexts = BIT(IWL_RXON_CTX_BSS);
    if (ucode_flags & IWL_UCODE_TLV_FLAGS_PAN)
        priv->valid_contexts |= BIT(IWL_RXON_CTX_PAN);
    
    for (i = 0; i < NUM_IWL_RXON_CTX; i++)
        priv->contexts[i].ctxid = (enum iwl_rxon_context_id)i;
    
    priv->contexts[IWL_RXON_CTX_BSS].always_active = true;
    priv->contexts[IWL_RXON_CTX_BSS].is_active = true;
    priv->contexts[IWL_RXON_CTX_BSS].rxon_cmd = REPLY_RXON;
    priv->contexts[IWL_RXON_CTX_BSS].rxon_timing_cmd = REPLY_RXON_TIMING;
    priv->contexts[IWL_RXON_CTX_BSS].rxon_assoc_cmd = REPLY_RXON_ASSOC;
    priv->contexts[IWL_RXON_CTX_BSS].qos_cmd = REPLY_QOS_PARAM;
    priv->contexts[IWL_RXON_CTX_BSS].ap_sta_id = IWL_AP_ID;
    priv->contexts[IWL_RXON_CTX_BSS].wep_key_cmd = REPLY_WEPKEY;
    priv->contexts[IWL_RXON_CTX_BSS].bcast_sta_id = IWLAGN_BROADCAST_ID;
    priv->contexts[IWL_RXON_CTX_BSS].exclusive_interface_modes = BIT(NL80211_IFTYPE_ADHOC)
                                                               | BIT(NL80211_IFTYPE_MONITOR);
    priv->contexts[IWL_RXON_CTX_BSS].interface_modes = BIT(NL80211_IFTYPE_STATION);
    priv->contexts[IWL_RXON_CTX_BSS].ap_devtype = RXON_DEV_TYPE_AP;
    priv->contexts[IWL_RXON_CTX_BSS].ibss_devtype = RXON_DEV_TYPE_IBSS;
    priv->contexts[IWL_RXON_CTX_BSS].station_devtype = RXON_DEV_TYPE_ESS;
    priv->contexts[IWL_RXON_CTX_BSS].unused_devtype = RXON_DEV_TYPE_ESS;
    memcpy(priv->contexts[IWL_RXON_CTX_BSS].ac_to_queue, iwlagn_bss_ac_to_queue, sizeof(iwlagn_bss_ac_to_queue));
    memcpy(priv->contexts[IWL_RXON_CTX_BSS].ac_to_fifo, iwlagn_bss_ac_to_fifo, sizeof(iwlagn_bss_ac_to_fifo));
    
    priv->contexts[IWL_RXON_CTX_PAN].rxon_cmd = REPLY_WIPAN_RXON;
    priv->contexts[IWL_RXON_CTX_PAN].rxon_timing_cmd = REPLY_WIPAN_RXON_TIMING;
    priv->contexts[IWL_RXON_CTX_PAN].rxon_assoc_cmd = REPLY_WIPAN_RXON_ASSOC;
    priv->contexts[IWL_RXON_CTX_PAN].qos_cmd = REPLY_WIPAN_QOS_PARAM;
    priv->contexts[IWL_RXON_CTX_PAN].ap_sta_id = IWL_AP_ID_PAN;
    priv->contexts[IWL_RXON_CTX_PAN].wep_key_cmd = REPLY_WIPAN_WEPKEY;
    priv->contexts[IWL_RXON_CTX_PAN].bcast_sta_id = IWLAGN_PAN_BCAST_ID;
    priv->contexts[IWL_RXON_CTX_PAN].station_flags = STA_FLG_PAN_STATION;
    priv->contexts[IWL_RXON_CTX_PAN].interface_modes = BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP);
    
    priv->contexts[IWL_RXON_CTX_PAN].ap_devtype = RXON_DEV_TYPE_CP;
    priv->contexts[IWL_RXON_CTX_PAN].station_devtype = RXON_DEV_TYPE_2STA;
    priv->contexts[IWL_RXON_CTX_PAN].unused_devtype = RXON_DEV_TYPE_P2P;
    memcpy(priv->contexts[IWL_RXON_CTX_PAN].ac_to_queue, iwlagn_pan_ac_to_queue, sizeof(iwlagn_pan_ac_to_queue));
    memcpy(priv->contexts[IWL_RXON_CTX_PAN].ac_to_fifo, iwlagn_pan_ac_to_fifo, sizeof(iwlagn_pan_ac_to_fifo));
    priv->contexts[IWL_RXON_CTX_PAN].mcast_queue = IWL_IPAN_MCAST_QUEUE;
    
    BUILD_BUG_ON(NUM_IWL_RXON_CTX != 2);
}

// line 678
static void iwl_rf_kill_ct_config(struct iwl_priv *priv)
{
    struct iwl_ct_kill_config cmd;
    struct iwl_ct_kill_throttling_config adv_cmd;
    int ret = 0;
    
    iwl_write32(priv->trans, CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_DRV_GP1_REG_BIT_CT_KILL_EXIT);
    
    priv->thermal_throttle.ct_kill_toggle = false;
    
    if (priv->lib->support_ct_kill_exit) {
        adv_cmd.critical_temperature_enter = cpu_to_le32(priv->hw_params.ct_kill_threshold);
        adv_cmd.critical_temperature_exit = cpu_to_le32(priv->hw_params.ct_kill_exit_threshold);
        
        ret = iwl_dvm_send_cmd_pdu(priv, REPLY_CT_KILL_CONFIG_CMD, 0, sizeof(adv_cmd), &adv_cmd);
        if (ret)
            IWL_ERR(priv, "REPLY_CT_KILL_CONFIG_CMD failed\n");
        else
            IWL_DEBUG_INFO(priv, "REPLY_CT_KILL_CONFIG_CMD succeeded, critical temperature enter is %d, exit is %d\n",
                           priv->hw_params.ct_kill_threshold, priv->hw_params.ct_kill_exit_threshold);
    } else {
        cmd.critical_temperature_R =
        cpu_to_le32(priv->hw_params.ct_kill_threshold);
        
        ret = iwl_dvm_send_cmd_pdu(priv,
                                   REPLY_CT_KILL_CONFIG_CMD,
                                   0, sizeof(cmd), &cmd);
        if (ret)
            IWL_ERR(priv, "REPLY_CT_KILL_CONFIG_CMD failed\n");
        else
            IWL_DEBUG_INFO(priv, "REPLY_CT_KILL_CONFIG_CMD succeeded, critical temperature is %d\n",
                           priv->hw_params.ct_kill_threshold);
    }
}


// line 723
static int iwlagn_send_calib_cfg_rt(struct iwl_priv *priv, u32 cfg)
{
    struct iwl_calib_cfg_cmd calib_cfg_cmd;
    struct iwl_host_cmd cmd = {
        .id = CALIBRATION_CFG_CMD,
        .len = { sizeof(struct iwl_calib_cfg_cmd), },
        .data = { &calib_cfg_cmd, },
    };
    
    memset(&calib_cfg_cmd, 0, sizeof(calib_cfg_cmd));
    calib_cfg_cmd.ucd_calib_cfg.once.is_enable = IWL_CALIB_RT_CFG_ALL;
    calib_cfg_cmd.ucd_calib_cfg.once.start = cpu_to_le32(cfg);
    
    return iwl_dvm_send_cmd(priv, &cmd);
}

// line 740
static int iwlagn_send_tx_ant_config(struct iwl_priv *priv, u8 valid_tx_ant)
{
    struct iwl_tx_ant_config_cmd tx_ant_cmd = {
        .valid = cpu_to_le32(valid_tx_ant),
    };
    
    if (IWL_UCODE_API(priv->fw->ucode_ver) > 1) {
        IWL_DEBUG_HC(priv, "select valid tx ant: %u\n", valid_tx_ant);
        
        return iwl_dvm_send_cmd_pdu(priv, TX_ANT_CONFIGURATION_CMD, 0, sizeof(struct iwl_tx_ant_config_cmd),
                                    &tx_ant_cmd);
    } else {
        IWL_DEBUG_HC(priv, "TX_ANT_CONFIGURATION_CMD not supported\n");
        return -EOPNOTSUPP;
    }
}

// line 757
static void iwl_send_bt_config(struct iwl_priv *priv)
{
    struct iwl_bt_cmd bt_cmd = {
        .lead_time = BT_LEAD_TIME_DEF,
        .max_kill = BT_MAX_KILL_DEF,
        .kill_ack_mask = 0,
        .kill_cts_mask = 0,
    };
    
    if (!iwlwifi_mod_params.bt_coex_active)
        bt_cmd.flags = BT_COEX_DISABLE;
    else
        bt_cmd.flags = BT_COEX_ENABLE;
    
    priv->bt_enable_flag = bt_cmd.flags;
    IWL_DEBUG_INFO(priv, "BT coex %s\n", (bt_cmd.flags == BT_COEX_DISABLE) ? "disable" : "active");

    if (iwl_dvm_send_cmd_pdu(priv, REPLY_BT_CONFIG, 0, sizeof(struct iwl_bt_cmd), &bt_cmd))
        IWL_ERR(priv, "failed to send BT Coex Config\n");
}



/** line 780
 * iwl_alive_start - called after REPLY_ALIVE notification received
 *                   from protocol/runtime uCode (initialization uCode's
 *                   Alive gets handled by iwl_init_alive_start()).
 */
int iwl_alive_start(struct iwl_priv *priv)
{
    int ret = 0;
    struct iwl_rxon_context *ctx = &priv->contexts[IWL_RXON_CTX_BSS];
    
    IWL_DEBUG_INFO(priv, "Runtime Alive received.\n");
    
    /* After the ALIVE response, we can send host commands to the uCode */
    set_bit(STATUS_ALIVE, &priv->status);
    
    if (iwl_is_rfkill(priv))
        return -ERFKILL;
    
    if (priv->event_log.ucode_trace) {
        /* start collecting data now */
        // TODO: Implement
        //mod_timer(&priv->ucode_trace, jiffies);
    }
    
    /* download priority table before any calibration request */
    if (priv->lib->bt_params && priv->lib->bt_params->advanced_bt_coexist) {
        /* Configure Bluetooth device coexistence support */
        if (priv->lib->bt_params->bt_sco_disable)
            priv->bt_enable_pspoll = false;
        else
            priv->bt_enable_pspoll = true;
        
        priv->bt_valid = IWLAGN_BT_ALL_VALID_MSK;
        priv->kill_ack_mask = IWLAGN_BT_KILL_ACK_MASK_DEFAULT;
        priv->kill_cts_mask = IWLAGN_BT_KILL_CTS_MASK_DEFAULT;
        iwlagn_send_advance_bt_config(priv);
        priv->bt_valid = IWLAGN_BT_VALID_ENABLE_FLAGS;
        priv->cur_rssi_ctx = NULL;
        
        iwl_send_prio_tbl(priv);
        
        /* FIXME: w/a to force change uCode BT state machine */
        ret = iwl_send_bt_env(priv, IWL_BT_COEX_ENV_OPEN, BT_COEX_PRIO_TBL_EVT_INIT_CALIB2);
        if (ret)
            return ret;
        ret = iwl_send_bt_env(priv, IWL_BT_COEX_ENV_CLOSE, BT_COEX_PRIO_TBL_EVT_INIT_CALIB2);
        if (ret)
            return ret;
    } else if (priv->lib->bt_params) {
        /*
         * default is 2-wire BT coexexistence support
         */
        iwl_send_bt_config(priv);
    }
    
    /*
     * Perform runtime calibrations, including DC calibration.
     */
    iwlagn_send_calib_cfg_rt(priv, IWL_CALIB_CFG_DC_IDX);
    
    // TODO: Implement
    //ieee80211_wake_queues(priv->hw);
    
    /* Configure Tx antenna selection based on H/W config */
    iwlagn_send_tx_ant_config(priv, priv->nvm_data->valid_tx_ant);
    
    if (iwl_is_associated_ctx(ctx) && !priv->wowlan) {
        struct iwl_rxon_cmd *active_rxon = (struct iwl_rxon_cmd *)&ctx->active;
        /* apply any changes in staging */
        ctx->staging.filter_flags |= RXON_FILTER_ASSOC_MSK;
        active_rxon->filter_flags &= ~RXON_FILTER_ASSOC_MSK;
    } else {
        struct iwl_rxon_context *tmp;
        /* Initialize our rx_config data */
        for_each_context(priv, tmp)
            iwl_connection_init_rx_config(priv, tmp);
        
        iwlagn_set_rxon_chain(priv, ctx);
    }
    
    if (!priv->wowlan) {
        /* WoWLAN ucode will not reply in the same way, skip it */
        iwl_reset_run_time_calib(priv);
    }
    
    set_bit(STATUS_READY, &priv->status);
    
    /* Configure the adapter for unassociated operation */
    ret = iwlagn_commit_rxon(priv, ctx);
    if (ret)
        return ret;
    
    /* At this point, the NIC is initialized and operational */
    iwl_rf_kill_ct_config(priv);
    
    IWL_DEBUG_INFO(priv, "ALIVE processing complete.\n");
    
    return iwl_power_update_mode(priv, true);
}

/** line 882
 * iwl_clear_driver_stations - clear knowledge of all stations from driver
 * @priv: iwl priv struct
 *
 * This is called during iwl_down() to make sure that in the case
 * we're coming there from a hardware restart mac80211 will be
 * able to reconfigure stations -- if we're getting there in the
 * normal down flow then the stations will already be cleared.
 */
static void iwl_clear_driver_stations(struct iwl_priv *priv)
{
    struct iwl_rxon_context *ctx;
    
    //IOSimpleLockLock(priv->sta_lock);
    memset(priv->stations, 0, sizeof(priv->stations));
    priv->num_stations = 0;
    
    priv->ucode_key_table = 0;
    
    for_each_context(priv, ctx) {
        /*
         * Remove all key information that is not stored as part
         * of station information since mac80211 may not have had
         * a chance to remove all the keys. When device is
         * reconfigured by mac80211 after an error all keys will
         * be reconfigured.
         */
        memset(ctx->wep_keys, 0, sizeof(ctx->wep_keys));
        ctx->key_mapping_keys = 0;
    }
    
    //IOSimpleLockUnlock(priv->sta_lock);
}


// line 916
void iwl_down(struct iwl_priv *priv)
{
    int exit_pending;
    
    IWL_DEBUG_INFO(priv, DRV_NAME " is going down\n");
    
    //lockdep_assert_held(&priv->mutex);
    
    iwl_scan_cancel_timeout(priv, 200);
    
    exit_pending = test_and_set_bit(STATUS_EXIT_PENDING, &priv->status);
    
    iwl_clear_ucode_stations(priv, NULL);
    iwl_dealloc_bcast_stations(priv);
    iwl_clear_driver_stations(priv);
    
    /* reset BT coex data */
    priv->bt_status = 0;
    priv->cur_rssi_ctx = NULL;
    priv->bt_is_sco = 0;
    if (priv->lib->bt_params)
        priv->bt_traffic_load = priv->lib->bt_params->bt_init_traffic_load;
    else
        priv->bt_traffic_load = 0;
    priv->bt_full_concurrent = false;
    priv->bt_ci_compliance = 0;
    
    /* Wipe out the EXIT_PENDING status bit if we are not actually
     * exiting the module */
    if (!exit_pending)
        clear_bit(STATUS_EXIT_PENDING, &priv->status);
    
//    if (priv->mac80211_registered)
//        ieee80211_stop_queues(priv->hw);
    
    priv->ucode_loaded = false;
    
    //iwl_trans_stop_device(priv->trans);
    
    /* Set num_aux_in_flight must be done after the transport is stopped */
    //atomic_set(&priv->num_aux_in_flight, 0);
    OSAddAtomic(-priv->num_aux_in_flight, &priv->num_aux_in_flight);
    
    /* Clear out all status bits but a few that are stable across reset */
    priv->status &= test_bit(STATUS_RF_KILL_HW, &priv->status) << STATUS_RF_KILL_HW
                  | test_bit(STATUS_FW_ERROR, &priv->status) << STATUS_FW_ERROR
                  | test_bit(STATUS_EXIT_PENDING, &priv->status) << STATUS_EXIT_PENDING;
    
    //dev_kfree_skb(priv->beacon_skb);
    //priv->beacon_skb = NULL;
}


// line 1112
static int iwl_init_drv(struct iwl_priv *priv)
{
    priv->sta_lock = IOSimpleLockAlloc();
 
    priv->mutex = IOLockAlloc();
    
    STAILQ_INIT(&priv->calib_results);
    
    priv->band = NL80211_BAND_2GHZ;
    
    priv->plcp_delta_threshold = priv->lib->plcp_delta_threshold;
    
    priv->iw_mode = NL80211_IFTYPE_STATION;
    priv->current_ht_config.smps = IEEE80211_SMPS_STATIC;
    priv->missed_beacon_threshold = IWL_MISSED_BEACON_THRESHOLD_DEF;
    priv->agg_tids_count = 0;
    
    priv->rx_statistics_jiffies = jiffies;
    
    /* Choose which receivers/antennas to use */
    iwlagn_set_rxon_chain(priv, &priv->contexts[IWL_RXON_CTX_BSS]);
    
    iwl_init_scan_params(priv);
    
    /* init bt coex */
    if (priv->lib->bt_params && priv->lib->bt_params->advanced_bt_coexist) {
        priv->kill_ack_mask = IWLAGN_BT_KILL_ACK_MASK_DEFAULT;
        priv->kill_cts_mask = IWLAGN_BT_KILL_CTS_MASK_DEFAULT;
        priv->bt_valid = IWLAGN_BT_ALL_VALID_MSK;
        priv->bt_on_thresh = BT_ON_THRESHOLD_DEF;
        priv->bt_duration = BT_DURATION_LIMIT_DEF;
        priv->dynamic_frag_thresh = BT_FRAG_THRESHOLD_DEF;
    }
    
    return 0;
}

// line 1150
static void iwl_uninit_drv(struct iwl_priv *priv)
{
    if (priv->scan_cmd) {
        iwh_free(priv->scan_cmd);
        priv->scan_cmd = NULL;
    }
    //kfree(priv->beacon_cmd);
//    kfree(rcu_dereference_raw(priv->noa_data));
    iwl_calib_free_results(priv);
//#ifdef CONFIG_IWLWIFI_DEBUGFS
//    kfree(priv->wowlan_sram);
//#endif
}


// line 1161
static void iwl_set_hw_params(struct iwl_priv *priv)
{
    if (priv->cfg->ht_params)
        priv->hw_params.use_rts_for_aggregation =
        priv->cfg->ht_params->use_rts_for_aggregation;
    
    /* Device-specific setup */
    priv->lib->set_hw_params(priv);
}

// line 1173
/* show what optional capabilities we have */
static void iwl_option_config(struct iwl_priv *priv)
{
#ifdef CONFIG_IWLWIFI_DEBUG
    IWL_INFO(priv, "CONFIG_IWLWIFI_DEBUG enabled\n");
#else
    IWL_INFO(priv, "CONFIG_IWLWIFI_DEBUG disabled\n");
#endif
    
#ifdef CONFIG_IWLWIFI_DEBUGFS
    IWL_INFO(priv, "CONFIG_IWLWIFI_DEBUGFS enabled\n");
#else
    IWL_INFO(priv, "CONFIG_IWLWIFI_DEBUGFS disabled\n");
#endif
    
#ifdef CONFIG_IWLWIFI_DEVICE_TRACING
    IWL_INFO(priv, "CONFIG_IWLWIFI_DEVICE_TRACING enabled\n");
#else
    IWL_INFO(priv, "CONFIG_IWLWIFI_DEVICE_TRACING disabled\n");
#endif
}



// line 1195
static int iwl_eeprom_init_hw_params(struct iwl_priv *priv)
{
    struct iwl_nvm_data *data = priv->nvm_data;
    
    if (data->sku_cap_11n_enable &&
        !priv->cfg->ht_params) {
        IWL_ERR(priv, "Invalid 11n configuration\n");
        return -EINVAL;
    }
    
    if (!data->sku_cap_11n_enable && !data->sku_cap_band_24GHz_enable && !data->sku_cap_band_52GHz_enable) {
        IWL_ERR(priv, "Invalid device sku\n");
        return -EINVAL;
    }
    
    IWL_DEBUG_INFO(priv,
                   "Device SKU: 24GHz %s %s, 52GHz %s %s, 11.n %s %s\n",
                   data->sku_cap_band_24GHz_enable ? "" : "NOT", "enabled",
                   data->sku_cap_band_52GHz_enable ? "" : "NOT", "enabled",
                   data->sku_cap_11n_enable ? "" : "NOT", "enabled");
    
    priv->hw_params.tx_chains_num = num_of_ant(data->valid_tx_ant);
    if (priv->cfg->rx_with_siso_diversity)
        priv->hw_params.rx_chains_num = 1;
    else
        priv->hw_params.rx_chains_num = num_of_ant(data->valid_rx_ant);
    
    IWL_DEBUG_INFO(priv, "Valid Tx ant: 0x%X, Valid Rx ant: 0x%X\n",
                   data->valid_tx_ant,
                   data->valid_rx_ant);
    
    return 0;
}


// line 1232
struct iwl_priv *IwlDvmOpMode::iwl_op_mode_dvm_start(struct iwl_trans *trans, const struct iwl_cfg *cfg,
                                                     const struct iwl_fw *fw)
{
    struct iwl_priv *priv;
    struct ieee80211_hw *hw;
    struct iwl_op_mode *op_mode;
    struct cfg80211_chan_def *def;
    u16 num_mac;
    u32 ucode_flags;
    struct iwl_trans_config trans_cfg = {};
    static const u8 no_reclaim_cmds[] = {
        REPLY_RX_PHY_CMD,
        REPLY_RX_MPDU_CMD,
        REPLY_COMPRESSED_BA,
        STATISTICS_NOTIFICATION,
        REPLY_TX,
    };
    int i;
    
    /************************
     * 1. Allocating HW data
     ************************/
    hw = iwl_alloc_all();
    if (!hw) {
        TraceLog("%s: Cannot allocate network device\n", cfg->name);
        goto out;
    }
    
//    op_mode = hw->priv;
//    op_mode->ops = &iwl_dvm_ops;
//    priv = IWL_OP_MODE_GET_DVM(op_mode);
    priv = (struct iwl_priv *)hw->priv;
    priv->trans = trans;
    //priv->dev = trans->dev;
    priv->cfg = cfg;
    priv->fw = fw;
    
    switch (priv->cfg->device_family) {
        case IWL_DEVICE_FAMILY_1000:
        case IWL_DEVICE_FAMILY_100:
            priv->lib = &iwl_dvm_1000_cfg;
            break;
        case IWL_DEVICE_FAMILY_2000:
            priv->lib = &iwl_dvm_2000_cfg;
            break;
        case IWL_DEVICE_FAMILY_105:
            priv->lib = &iwl_dvm_105_cfg;
            break;
        case IWL_DEVICE_FAMILY_2030:
        case IWL_DEVICE_FAMILY_135:
            priv->lib = &iwl_dvm_2030_cfg;
            break;
        case IWL_DEVICE_FAMILY_5000:
            priv->lib = &iwl_dvm_5000_cfg;
            break;
        case IWL_DEVICE_FAMILY_5150:
            priv->lib = &iwl_dvm_5150_cfg;
            break;
        case IWL_DEVICE_FAMILY_6000:
        case IWL_DEVICE_FAMILY_6000i:
            priv->lib = &iwl_dvm_6000_cfg;
            break;
        case IWL_DEVICE_FAMILY_6005:
            priv->lib = &iwl_dvm_6005_cfg;
            break;
        case IWL_DEVICE_FAMILY_6050:
        case IWL_DEVICE_FAMILY_6150:
            priv->lib = &iwl_dvm_6050_cfg;
            break;
        case IWL_DEVICE_FAMILY_6030:
            priv->lib = &iwl_dvm_6030_cfg;
            break;
        default:
            break;
    }
    
    if (WARN_ON(!priv->lib))
        goto out_free_hw;
    
    /*
     * Populate the state variables that the transport layer needs
     * to know about.
     */
//    trans_cfg.op_mode = op_mode;
    trans_cfg.no_reclaim_cmds = no_reclaim_cmds;
    trans_cfg.n_no_reclaim_cmds = ARRAY_SIZE(no_reclaim_cmds);
    
    switch (iwlwifi_mod_params.amsdu_size) {
        case IWL_AMSDU_DEF:
        case IWL_AMSDU_4K:
            trans_cfg.rx_buf_size = IWL_AMSDU_4K;
            break;
        case IWL_AMSDU_8K:
            trans_cfg.rx_buf_size = IWL_AMSDU_8K;
            break;
        case IWL_AMSDU_12K:
        default:
            trans_cfg.rx_buf_size = IWL_AMSDU_4K;
            TraceLog("Unsupported amsdu_size: %d\n", iwlwifi_mod_params.amsdu_size);
    }
    
    trans_cfg.cmd_q_wdg_timeout = IWL_WATCHDOG_DISABLED;
    
    trans_cfg.command_groups = iwl_dvm_groups;
    trans_cfg.command_groups_size = ARRAY_SIZE(iwl_dvm_groups);
    
    trans_cfg.cmd_fifo = IWLAGN_CMD_FIFO_NUM;
    // TODO: Implement
    trans_cfg.cb_data_offs = 0;//offsetof(struct ieee80211_tx_info, driver_data[2]);
    
    if (sizeof(priv->transport_queue_stop) * BITS_PER_BYTE < priv->cfg->base_params->num_of_queues) {
        IWL_WARN(trans, "sizeof(priv->transport_queue_stop) * BITS_PER_BYTE < priv->cfg->base_params->num_of_queues");
    }
    
    ucode_flags = fw->ucode_capa.flags;
    
    if (ucode_flags & IWL_UCODE_TLV_FLAGS_PAN) {
        priv->sta_key_max_num = STA_KEY_MAX_NUM_PAN;
        trans_cfg.cmd_queue = IWL_IPAN_CMD_QUEUE_NUM;
    } else {
        priv->sta_key_max_num = STA_KEY_MAX_NUM;
        trans_cfg.cmd_queue = IWL_DEFAULT_CMD_QUEUE_NUM;
    }
    
    /* Configure transport layer */
    _ops->configure(priv->trans, &trans_cfg);
    
    trans->rx_mpdu_cmd = REPLY_RX_MPDU_CMD;
    trans->rx_mpdu_cmd_hdr_size = sizeof(struct iwl_rx_mpdu_res_start);
    trans->command_groups = trans_cfg.command_groups;
    trans->command_groups_size = trans_cfg.command_groups_size;
    
    /* At this point both hw and priv are allocated. */
    
    //SET_IEEE80211_DEV(priv->hw, priv->trans->dev);
    
    iwl_option_config(priv);
    
    IWL_DEBUG_INFO(priv, "*** LOAD DRIVER ***\n");
    
    /* is antenna coupling more than 35dB ? */
    priv->bt_ant_couple_ok = (iwlwifi_mod_params.antenna_coupling > IWL_BT_ANTENNA_COUPLING_THRESHOLD) ? true : false;
    
    /* bt channel inhibition enabled*/
    priv->bt_ch_announce = true;
    IWL_DEBUG_INFO(priv, "BT channel inhibition is %s\n", (priv->bt_ch_announce) ? "On" : "Off");
    
    /* these spin locks will be used in apm_ops.init and EEPROM access
     * we should init now
     */
    priv->statistics.lock = IOSimpleLockAlloc();
    
    /***********************
     * 2. Read REV register
     ***********************/
    IWL_INFO(priv, "Detected %s, REV=0x%X\n", priv->cfg->name, priv->trans->hw_rev);
    
    if (_ops->start_hw(priv->trans, true))
        goto out_free_hw;
    
    /* Read the EEPROM */
    if (iwl_read_eeprom(priv->trans, &priv->eeprom_blob, &priv->eeprom_blob_size)) {
        IWL_ERR(priv, "Unable to init EEPROM\n");
        goto out_free_hw;
    }
    
    /* Reset chip to save power until we load uCode during "up". */
    _ops->stop_device(priv->trans, true);
    
    priv->nvm_data = iwl_parse_eeprom_data(NULL, priv->cfg, priv->eeprom_blob, priv->eeprom_blob_size);
    
    if (!priv->nvm_data)
        goto out_free_eeprom_blob;

    if (iwl_nvm_check_version(priv->nvm_data, priv->trans))
        goto out_free_eeprom;
    
    if (iwl_eeprom_init_hw_params(priv))
        goto out_free_eeprom;
    
    /* extract MAC Address */
    memcpy(priv->addresses[0].addr, priv->nvm_data->hw_addr, ETH_ALEN);
    IWL_DEBUG_INFO(priv, "MAC address: " MAC_FMT "\n", MAC_BYTES(priv->addresses[0].addr));
    priv->hw->wiphy->addresses = priv->addresses;
    priv->hw->wiphy->n_addresses = 1;
    num_mac = priv->nvm_data->n_hw_addrs;
    if (num_mac > 1) {
        memcpy(priv->addresses[1].addr, priv->addresses[0].addr, ETH_ALEN);
        priv->addresses[1].addr[5]++;
        priv->hw->wiphy->n_addresses++;
    }
    
    // TODO: This was added by me. Without it, channel is not initialized and rxon commit fails
    def = (struct cfg80211_chan_def *)iwh_malloc(sizeof(struct cfg80211_chan_def));
    def->chan = &priv->nvm_data->channels[3];
    def->width =  NL80211_CHAN_WIDTH_40;
    def->center_freq1 = def->chan->center_freq;
    
    priv->hw->conf.chandef = *def;

    /************************
     * 4. Setup HW constants
     ************************/
    iwl_set_hw_params(priv);

    if (!(priv->nvm_data->sku_cap_ipan_enable)) {
        IWL_DEBUG_INFO(priv, "Your EEPROM disabled PAN\n");
        ucode_flags &= ~IWL_UCODE_TLV_FLAGS_PAN;
        /*
         * if not PAN, then don't support P2P -- might be a uCode
         * packaging bug or due to the eeprom check above
         */
        priv->sta_key_max_num = STA_KEY_MAX_NUM;
        trans_cfg.cmd_queue = IWL_DEFAULT_CMD_QUEUE_NUM;

        /* Configure transport layer again*/
        _ops->configure(priv->trans, &trans_cfg);
    }

    /*******************
     * 5. Setup priv
     *******************/
    for (i = 0; i < IWL_MAX_HW_QUEUES; i++) {
        priv->queue_to_mac80211[i] = IWL_INVALID_MAC80211_QUEUE;
        if (i < IWLAGN_FIRST_AMPDU_QUEUE && i != IWL_DEFAULT_CMD_QUEUE_NUM && i != IWL_IPAN_CMD_QUEUE_NUM)
            priv->queue_to_mac80211[i] = i;
        
        priv->queue_stop_count[i] = 0;
    }

    if (iwl_init_drv(priv))
        goto out_free_eeprom;

    /* At this point both hw and priv are initialized. */

    /********************
     * 6. Setup services
     ********************/
//    iwl_setup_deferred_work(priv);
    iwl_setup_rx_handlers(priv);
    iwl_power_initialize(priv);
    iwl_tt_initialize(priv);

    snprintf(priv->hw->wiphy->fw_version, sizeof(priv->hw->wiphy->fw_version), "%s", fw->fw_version);

    priv->new_scan_threshold_behaviour = !!(ucode_flags & IWL_UCODE_TLV_FLAGS_NEWSCAN);

    priv->phy_calib_chain_noise_reset_cmd = fw->ucode_capa.standard_phy_calibration_size;
    priv->phy_calib_chain_noise_gain_cmd = fw->ucode_capa.standard_phy_calibration_size + 1;

    /* initialize all valid contexts */
    iwl_init_context(priv, ucode_flags);

    /**************************************************
     * This is still part of probe() in a sense...
     *
     * 7. Setup and register with mac80211 and debugfs
     **************************************************/
    if (iwlagn_mac_setup_register(priv, &fw->ucode_capa))
        goto out_destroy_workqueue;

//    if (iwl_dbgfs_register(priv, dbgfs_dir))
//        goto out_mac80211_unregister;
    
    return priv;
    
out_mac80211_unregister:
    //iwlagn_mac_unregister(priv);
out_destroy_workqueue:
    iwl_tt_exit(priv);
//    iwl_cancel_deferred_work(priv);
//    destroy_workqueue(priv->workqueue);
    priv->workqueue = NULL;
    iwl_uninit_drv(priv);
out_free_eeprom_blob:
    iwh_free(priv->eeprom_blob);
out_free_eeprom:
    iwh_free(priv->nvm_data);
out_free_hw:
//    ieee80211_free_hw(priv->hw);
out:
    op_mode = NULL;
    return NULL;
}

// line 1524
void IwlDvmOpMode::iwl_op_mode_dvm_stop(struct iwl_priv* priv)
{
    IWL_DEBUG_INFO(priv, "*** UNLOAD DRIVER ***\n");
    
//    iwlagn_mac_unregister(priv);

    iwl_tt_exit(priv);

    iwh_free((void *)priv->eeprom_blob);
    iwh_free(priv->nvm_data);

    /*netif_stop_queue(dev); */
    //flush_workqueue(priv->workqueue);

    /* ieee80211_unregister_hw calls iwlagn_mac_stop, which flushes
     * priv->workqueue... so we can't take down the workqueue
     * until now... */
    //destroy_workqueue(priv->workqueue);
    priv->workqueue = NULL;

    iwl_uninit_drv(priv);

    //dev_kfree_skb(priv->beacon_skb);

    _ops->op_mode_leave(priv->trans);
    
    //ieee80211_free_hw(priv->hw);
}


#define EEPROM_RF_CONFIG_TYPE_MAX      0x3

// line 1996
void IwlDvmOpMode::iwl_nic_config(struct iwl_priv *priv)
{
    //struct iwl_priv *priv = IWL_OP_MODE_GET_DVM(op_mode);
    
    /* SKU Control */
    iwl_trans_set_bits_mask(priv->trans, CSR_HW_IF_CONFIG_REG,
                            CSR_HW_IF_CONFIG_REG_MSK_MAC_DASH |
                            CSR_HW_IF_CONFIG_REG_MSK_MAC_STEP,
                            (CSR_HW_REV_STEP(priv->trans->hw_rev) <<
                             CSR_HW_IF_CONFIG_REG_POS_MAC_STEP) |
                            (CSR_HW_REV_DASH(priv->trans->hw_rev) <<
                             CSR_HW_IF_CONFIG_REG_POS_MAC_DASH));
    
    /* write radio config values to register */
    if (priv->nvm_data->radio_cfg_type <= EEPROM_RF_CONFIG_TYPE_MAX) {
        u32 reg_val =
            priv->nvm_data->radio_cfg_type << CSR_HW_IF_CONFIG_REG_POS_PHY_TYPE |
            priv->nvm_data->radio_cfg_step << CSR_HW_IF_CONFIG_REG_POS_PHY_STEP |
            priv->nvm_data->radio_cfg_dash << CSR_HW_IF_CONFIG_REG_POS_PHY_DASH;
        
        iwl_trans_set_bits_mask(priv->trans, CSR_HW_IF_CONFIG_REG,
                                CSR_HW_IF_CONFIG_REG_MSK_PHY_TYPE |
                                CSR_HW_IF_CONFIG_REG_MSK_PHY_STEP |
                                CSR_HW_IF_CONFIG_REG_MSK_PHY_DASH,
                                reg_val);
        
        IWL_INFO(priv, "Radio type=0x%x-0x%x-0x%x\n",
                 priv->nvm_data->radio_cfg_type,
                 priv->nvm_data->radio_cfg_step,
                 priv->nvm_data->radio_cfg_dash);
    } else {
        //WARN_ON(1);
    }
    
    /* set CSR_HW_CONFIG_REG for uCode use */
    iwl_set_bit(priv->trans, CSR_HW_IF_CONFIG_REG,
                CSR_HW_IF_CONFIG_REG_BIT_RADIO_SI |
                CSR_HW_IF_CONFIG_REG_BIT_MAC_SI);
    
    /* W/A : NIC is stuck in a reset state after Early PCIe power off
     * (PCIe power is lost before PERST# is asserted),
     * causing ME FW to lose ownership and not being able to obtain it back.
     */
    iwl_set_bits_mask_prph(priv->trans, APMG_PS_CTRL_REG,
                           APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS,
                           ~APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS);
    
    if (priv->lib->nic_config)
        priv->lib->nic_config(priv);
}



