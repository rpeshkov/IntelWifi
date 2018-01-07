//
//  IwlDvmOpMode.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

extern "C" {
#include "iwlwifi/dvm/commands.h"
#include "iwlwifi/dvm/dev.h"
#include "iwlwifi/dvm/agn.h"
#include "iwlwifi/iwl-drv.h"

}

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_BYTES(x) (x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5]

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


IwlDvmOpMode::IwlDvmOpMode(IwlTransOps *ops, IntelEeprom *eeprom) {
    _ops = ops;
    _eeprom = eeprom;
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
    priv->contexts[IWL_RXON_CTX_BSS].exclusive_interface_modes =
    BIT(NL80211_IFTYPE_ADHOC) | BIT(NL80211_IFTYPE_MONITOR);
    priv->contexts[IWL_RXON_CTX_BSS].interface_modes =
    BIT(NL80211_IFTYPE_STATION);
    priv->contexts[IWL_RXON_CTX_BSS].ap_devtype = RXON_DEV_TYPE_AP;
    priv->contexts[IWL_RXON_CTX_BSS].ibss_devtype = RXON_DEV_TYPE_IBSS;
    priv->contexts[IWL_RXON_CTX_BSS].station_devtype = RXON_DEV_TYPE_ESS;
    priv->contexts[IWL_RXON_CTX_BSS].unused_devtype = RXON_DEV_TYPE_ESS;
    memcpy(priv->contexts[IWL_RXON_CTX_BSS].ac_to_queue,
           iwlagn_bss_ac_to_queue, sizeof(iwlagn_bss_ac_to_queue));
    memcpy(priv->contexts[IWL_RXON_CTX_BSS].ac_to_fifo,
           iwlagn_bss_ac_to_fifo, sizeof(iwlagn_bss_ac_to_fifo));
    
    priv->contexts[IWL_RXON_CTX_PAN].rxon_cmd = REPLY_WIPAN_RXON;
    priv->contexts[IWL_RXON_CTX_PAN].rxon_timing_cmd =
    REPLY_WIPAN_RXON_TIMING;
    priv->contexts[IWL_RXON_CTX_PAN].rxon_assoc_cmd =
    REPLY_WIPAN_RXON_ASSOC;
    priv->contexts[IWL_RXON_CTX_PAN].qos_cmd = REPLY_WIPAN_QOS_PARAM;
    priv->contexts[IWL_RXON_CTX_PAN].ap_sta_id = IWL_AP_ID_PAN;
    priv->contexts[IWL_RXON_CTX_PAN].wep_key_cmd = REPLY_WIPAN_WEPKEY;
    priv->contexts[IWL_RXON_CTX_PAN].bcast_sta_id = IWLAGN_PAN_BCAST_ID;
    priv->contexts[IWL_RXON_CTX_PAN].station_flags = STA_FLG_PAN_STATION;
    priv->contexts[IWL_RXON_CTX_PAN].interface_modes =
    BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP);
    
    priv->contexts[IWL_RXON_CTX_PAN].ap_devtype = RXON_DEV_TYPE_CP;
    priv->contexts[IWL_RXON_CTX_PAN].station_devtype = RXON_DEV_TYPE_2STA;
    priv->contexts[IWL_RXON_CTX_PAN].unused_devtype = RXON_DEV_TYPE_P2P;
    memcpy(priv->contexts[IWL_RXON_CTX_PAN].ac_to_queue,
           iwlagn_pan_ac_to_queue, sizeof(iwlagn_pan_ac_to_queue));
    memcpy(priv->contexts[IWL_RXON_CTX_PAN].ac_to_fifo,
           iwlagn_pan_ac_to_fifo, sizeof(iwlagn_pan_ac_to_fifo));
    priv->contexts[IWL_RXON_CTX_PAN].mcast_queue = IWL_IPAN_MCAST_QUEUE;
    
    //BUILD_BUG_ON(NUM_IWL_RXON_CTX != 2);
}


// line 1112
static int iwl_init_drv(struct iwl_priv *priv)
{
    priv->sta_lock = IOSimpleLockAlloc();
 
    priv->mutex = IOLockAlloc();
    
    INIT_LIST_HEAD(&priv->calib_results);
    
    priv->band = NL80211_BAND_2GHZ;
    
    priv->plcp_delta_threshold = priv->lib->plcp_delta_threshold;
    
    priv->iw_mode = NL80211_IFTYPE_STATION;
    priv->current_ht_config.smps = IEEE80211_SMPS_STATIC;
    priv->missed_beacon_threshold = IWL_MISSED_BEACON_THRESHOLD_DEF;
    priv->agg_tids_count = 0;
    
    //priv->rx_statistics_jiffies = jiffies;
    
    /* Choose which receivers/antennas to use */
    //iwlagn_set_rxon_chain(priv, &priv->contexts[IWL_RXON_CTX_BSS]);
    
    //iwl_init_scan_params(priv);
    
    /* init bt coex */
    if (priv->lib->bt_params &&
        priv->lib->bt_params->advanced_bt_coexist) {
        priv->kill_ack_mask = IWLAGN_BT_KILL_ACK_MASK_DEFAULT;
        priv->kill_cts_mask = IWLAGN_BT_KILL_CTS_MASK_DEFAULT;
        priv->bt_valid = IWLAGN_BT_ALL_VALID_MSK;
        priv->bt_on_thresh = BT_ON_THRESHOLD_DEF;
        priv->bt_duration = BT_DURATION_LIMIT_DEF;
        priv->dynamic_frag_thresh = BT_FRAG_THRESHOLD_DEF;
    }
    
    return 0;
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


// line 1195
static int iwl_eeprom_init_hw_params(struct iwl_priv *priv)
{
    struct iwl_nvm_data *data = priv->nvm_data;
    
    if (data->sku_cap_11n_enable &&
        !priv->cfg->ht_params) {
        IWL_ERR(priv, "Invalid 11n configuration\n");
        return -EINVAL;
    }
    
    if (!data->sku_cap_11n_enable && !data->sku_cap_band_24GHz_enable &&
        !data->sku_cap_band_52GHz_enable) {
        IWL_ERR(priv, "Invalid device sku\n");
        return -EINVAL;
    }
    
    IWL_DEBUG_INFO(priv,
                   "Device SKU: 24GHz %s %s, 52GHz %s %s, 11.n %s %s\n",
                   data->sku_cap_band_24GHz_enable ? "" : "NOT", "enabled",
                   data->sku_cap_band_52GHz_enable ? "" : "NOT", "enabled",
                   data->sku_cap_11n_enable ? "" : "NOT", "enabled");
    
    priv->hw_params.tx_chains_num =
    num_of_ant(data->valid_tx_ant);
    if (priv->cfg->rx_with_siso_diversity)
        priv->hw_params.rx_chains_num = 1;
    else
        priv->hw_params.rx_chains_num =
        num_of_ant(data->valid_rx_ant);
    
    IWL_DEBUG_INFO(priv, "Valid Tx ant: 0x%X, Valid Rx ant: 0x%X\n",
                   data->valid_tx_ant,
                   data->valid_rx_ant);
    
    return 0;
}


// line 1232
struct iwl_priv *IwlDvmOpMode::iwl_op_mode_dvm_start(struct iwl_trans *trans, const struct iwl_cfg *cfg, const struct iwl_fw *fw, struct dentry *dbgfs_dir)
{
    struct iwl_priv *priv;
//    struct ieee80211_hw *hw;
    struct iwl_op_mode *op_mode;
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
//    hw = iwl_alloc_all();
//    if (!hw) {
//        TraceLog("%s: Cannot allocate network device\n", cfg->name);
//        goto out;
//    }
    
//    op_mode = hw->priv;
//    op_mode->ops = &iwl_dvm_ops;
//    priv = IWL_OP_MODE_GET_DVM(op_mode);
    priv = (struct iwl_priv *)IOMalloc(sizeof(iwl_priv));
    priv->trans = trans;
    priv->dev = trans->dev;
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
            TraceLog("Unsupported amsdu_size: %d\n",
                   iwlwifi_mod_params.amsdu_size);
    }
    
    trans_cfg.cmd_q_wdg_timeout = IWL_WATCHDOG_DISABLED;
    
    trans_cfg.command_groups = iwl_dvm_groups;
    trans_cfg.command_groups_size = ARRAY_SIZE(iwl_dvm_groups);
    
    trans_cfg.cmd_fifo = IWLAGN_CMD_FIFO_NUM;
    // TODO: Implement
//    trans_cfg.cb_data_offs = offsetof(struct ieee80211_tx_info,
//                                      driver_data[2]);
    
//    WARN_ON(sizeof(priv->transport_queue_stop) * BITS_PER_BYTE <
//            priv->cfg->base_params->num_of_queues);
    
    ucode_flags = fw->ucode_capa.flags;
    
    if (ucode_flags & IWL_UCODE_TLV_FLAGS_PAN) {
        priv->sta_key_max_num = STA_KEY_MAX_NUM_PAN;
        trans_cfg.cmd_queue = IWL_IPAN_CMD_QUEUE_NUM;
    } else {
        priv->sta_key_max_num = STA_KEY_MAX_NUM;
        trans_cfg.cmd_queue = IWL_DEFAULT_CMD_QUEUE_NUM;
    }
    
    /* Configure transport layer */
    //iwl_trans_configure(priv->trans, &trans_cfg);
    _ops->configure(priv->trans, &trans_cfg);
    
    trans->rx_mpdu_cmd = REPLY_RX_MPDU_CMD;
    trans->rx_mpdu_cmd_hdr_size = sizeof(struct iwl_rx_mpdu_res_start);
    trans->command_groups = trans_cfg.command_groups;
    trans->command_groups_size = trans_cfg.command_groups_size;
    
    /* At this point both hw and priv are allocated. */
    
    //SET_IEEE80211_DEV(priv->hw, priv->trans->dev);
    
    // TODO: Implement
    //iwl_option_config(priv);
    
    IWL_DEBUG_INFO(priv, "*** LOAD DRIVER ***\n");
    
    /* is antenna coupling more than 35dB ? */
    priv->bt_ant_couple_ok =
    (iwlwifi_mod_params.antenna_coupling >
     IWL_BT_ANTENNA_COUPLING_THRESHOLD) ?
    true : false;
    
    /* bt channel inhibition enabled*/
    priv->bt_ch_announce = true;
    IWL_DEBUG_INFO(priv, "BT channel inhibition is %s\n",
                   (priv->bt_ch_announce) ? "On" : "Off");
    
    /* these spin locks will be used in apm_ops.init and EEPROM access
     * we should init now
     */
    priv->statistics.lock = IOSimpleLockAlloc();
    
    /***********************
     * 2. Read REV register
     ***********************/
    IWL_INFO(priv, "Detected %s, REV=0x%X\n",
             priv->cfg->name, priv->trans->hw_rev);
    
    if (_ops->start_hw(priv->trans, true))
        goto out_free_hw;
    
    /* Read the EEPROM */
    if (!_eeprom->read()) {
        IWL_ERR(priv, "Unable to init EEPROM\n");
        goto out_free_hw;
    }
    
    /* Reset chip to save power until we load uCode during "up". */
    _ops->stop_device(priv->trans, true);
    
    priv->nvm_data = _eeprom->parse();
    
    if (!priv->nvm_data)
        goto out_free_eeprom_blob;

    // TODO: Implement
    if (_eeprom->iwl_nvm_check_version(priv->nvm_data, priv->trans))
        goto out_free_eeprom;
    
    if (iwl_eeprom_init_hw_params(priv))
        goto out_free_eeprom;
    
    /* extract MAC Address */
    memcpy(priv->addresses[0].addr, priv->nvm_data->hw_addr, ETH_ALEN);
    IWL_DEBUG_INFO(priv, "MAC address: " MAC_FMT "\n", MAC_BYTES(priv->addresses[0].addr));
//    priv->hw->wiphy->addresses = priv->addresses;
//    priv->hw->wiphy->n_addresses = 1;
    num_mac = priv->nvm_data->n_hw_addrs;
    if (num_mac > 1) {
        memcpy(priv->addresses[1].addr, priv->addresses[0].addr,
               ETH_ALEN);
        priv->addresses[1].addr[5]++;
//        priv->hw->wiphy->n_addresses++;
    }

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
        if (i < IWLAGN_FIRST_AMPDU_QUEUE &&
            i != IWL_DEFAULT_CMD_QUEUE_NUM &&
            i != IWL_IPAN_CMD_QUEUE_NUM)
            priv->queue_to_mac80211[i] = i;
        //atomic_set(&priv->queue_stop_count[i], 0);
        priv->queue_stop_count[i] = 0;
    }

    if (iwl_init_drv(priv))
        goto out_free_eeprom;

    /* At this point both hw and priv are initialized. */

    /********************
     * 6. Setup services
     ********************/
////    iwl_setup_deferred_work(priv);
//    iwl_setup_rx_handlers(priv);
    iwl_notification_wait_init(&priv->notif_wait);

//
    iwl_power_initialize(priv);
////    iwl_tt_initialize(priv);
//
////    snprintf(priv->hw->wiphy->fw_version,
////             sizeof(priv->hw->wiphy->fw_version),
////             "%s", fw->fw_version);
//
    priv->new_scan_threshold_behaviour =
    !!(ucode_flags & IWL_UCODE_TLV_FLAGS_NEWSCAN);

    priv->phy_calib_chain_noise_reset_cmd =
    fw->ucode_capa.standard_phy_calibration_size;
    priv->phy_calib_chain_noise_gain_cmd =
    fw->ucode_capa.standard_phy_calibration_size + 1;

    /* initialize all valid contexts */
    iwl_init_context(priv, ucode_flags);

    /**************************************************
     * This is still part of probe() in a sense...
     *
     * 7. Setup and register with mac80211 and debugfs
     **************************************************/
////    if (iwlagn_mac_setup_register(priv, &fw->ucode_capa))
////        goto out_destroy_workqueue;
////
////    if (iwl_dbgfs_register(priv, dbgfs_dir))
////        goto out_mac80211_unregister;
    
    return priv;
    
out_mac80211_unregister:
    //iwlagn_mac_unregister(priv);
out_destroy_workqueue:
//    iwl_tt_exit(priv);
//    iwl_cancel_deferred_work(priv);
//    destroy_workqueue(priv->workqueue);
//    priv->workqueue = NULL;
//    iwl_uninit_drv(priv);
out_free_eeprom_blob:
//    kfree(priv->eeprom_blob);
out_free_eeprom:
//    kfree(priv->nvm_data);
out_free_hw:
//    ieee80211_free_hw(priv->hw);
out:
    op_mode = NULL;
    return NULL;
}

