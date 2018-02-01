//
//  IwlDvmOpMode_mac80211.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

extern "C" {
#include "iwlwifi/dvm/agn.h"
#include "iwlwifi/dvm/dev.h"
}


/*****************************************************************************
 *
 * mac80211 entry point functions
 *
 *****************************************************************************/

static const struct ieee80211_iface_limit iwlagn_sta_ap_limits[] = {
    {
        .max = 1,
        .types = BIT(NL80211_IFTYPE_STATION),
    },
    {
        .max = 1,
        .types = BIT(NL80211_IFTYPE_AP),
    },
};

static const struct ieee80211_iface_limit iwlagn_2sta_limits[] = {
    {
        .max = 2,
        .types = BIT(NL80211_IFTYPE_STATION),
    },
};

static const struct ieee80211_iface_combination
iwlagn_iface_combinations_dualmode[] = {
    { .num_different_channels = 1,
        .max_interfaces = 2,
        .beacon_int_infra_match = true,
        .limits = iwlagn_sta_ap_limits,
        .n_limits = ARRAY_SIZE(iwlagn_sta_ap_limits),
    },
    { .num_different_channels = 1,
        .max_interfaces = 2,
        .limits = iwlagn_2sta_limits,
        .n_limits = ARRAY_SIZE(iwlagn_2sta_limits),
    },
};

/* ieee80211_radiotap.h:117 for IEEE80211_RADIOTAP_MCS "have" flags */
enum ieee80211_radiotap_mcs_have {
    IEEE80211_RADIOTAP_MCS_HAVE_BW = 0x01,
    IEEE80211_RADIOTAP_MCS_HAVE_MCS = 0x02,
    IEEE80211_RADIOTAP_MCS_HAVE_GI = 0x04,
    IEEE80211_RADIOTAP_MCS_HAVE_FMT = 0x08,
    IEEE80211_RADIOTAP_MCS_HAVE_FEC = 0x10,
    IEEE80211_RADIOTAP_MCS_HAVE_STBC = 0x20,
};



/* line 93
 * Not a mac80211 entry point function, but it fits in with all the
 * other mac80211 functions grouped here.
 */
int iwlagn_mac_setup_register(struct iwl_priv *priv,
                              const struct iwl_ucode_capabilities *capa)
{
    int ret;
    struct ieee80211_hw *hw = priv->hw;
    struct iwl_rxon_context *ctx;
    
    hw->rate_control_algorithm = "iwl-agn-rs";
    
    /* Tell mac80211 our characteristics */
//    ieee80211_hw_set(hw, SIGNAL_DBM);
//    ieee80211_hw_set(hw, AMPDU_AGGREGATION);
//    ieee80211_hw_set(hw, NEED_DTIM_BEFORE_ASSOC);
//    ieee80211_hw_set(hw, SPECTRUM_MGMT);
//    ieee80211_hw_set(hw, REPORTS_TX_ACK_STATUS);
//    ieee80211_hw_set(hw, QUEUE_CONTROL);
//    ieee80211_hw_set(hw, SUPPORTS_PS);
//    ieee80211_hw_set(hw, SUPPORTS_DYNAMIC_PS);
//    ieee80211_hw_set(hw, SUPPORT_FAST_XMIT);
//    ieee80211_hw_set(hw, WANT_MONITOR_VIF);
    
    if (priv->trans->max_skb_frags)
        hw->netdev_features = NETIF_F_HIGHDMA | NETIF_F_SG;
    
    hw->offchannel_tx_hw_queue = IWL_AUX_QUEUE;
    hw->radiotap_mcs_details |= IEEE80211_RADIOTAP_MCS_HAVE_FMT;
    
    /*
     * Including the following line will crash some AP's.  This
     * workaround removes the stimulus which causes the crash until
     * the AP software can be fixed.
     hw->max_tx_aggregation_subframes = LINK_QUAL_AGG_FRAME_LIMIT_DEF;
     */
    
    if (priv->nvm_data->sku_cap_11n_enable)
        hw->wiphy->features |= NL80211_FEATURE_DYNAMIC_SMPS |
        NL80211_FEATURE_STATIC_SMPS;
    
    /*
     * Enable 11w if advertised by firmware and software crypto
     * is not enabled (as the firmware will interpret some mgmt
     * packets, so enabling it with software crypto isn't safe)
     */
//    if (priv->fw->ucode_capa.flags & IWL_UCODE_TLV_FLAGS_MFP &&
//        !iwlwifi_mod_params.swcrypto)
//        ieee80211_hw_set(hw, MFP_CAPABLE);
    
    hw->sta_data_size = sizeof(struct iwl_station_priv);
    hw->vif_data_size = sizeof(struct iwl_vif_priv);
    
    for_each_context(priv, ctx) {
        hw->wiphy->interface_modes |= ctx->interface_modes;
        hw->wiphy->interface_modes |= ctx->exclusive_interface_modes;
    }
    
    //BUILD_BUG_ON(NUM_IWL_RXON_CTX != 2);
    
    if (hw->wiphy->interface_modes & BIT(NL80211_IFTYPE_AP)) {
        hw->wiphy->iface_combinations = iwlagn_iface_combinations_dualmode;
        hw->wiphy->n_iface_combinations = ARRAY_SIZE(iwlagn_iface_combinations_dualmode);
    }
    
    hw->wiphy->flags |= WIPHY_FLAG_IBSS_RSN;
    hw->wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG | REGULATORY_DISABLE_BEACON_HINTS;
    
#ifdef CONFIG_PM_SLEEP
    if (priv->fw->img[IWL_UCODE_WOWLAN].num_sec &&
        priv->trans->ops->d3_suspend &&
        priv->trans->ops->d3_resume &&
        device_can_wakeup(priv->trans->dev)) {
        priv->wowlan_support.flags = WIPHY_WOWLAN_MAGIC_PKT |
        WIPHY_WOWLAN_DISCONNECT |
        WIPHY_WOWLAN_EAP_IDENTITY_REQ |
        WIPHY_WOWLAN_RFKILL_RELEASE;
        if (!iwlwifi_mod_params.swcrypto)
            priv->wowlan_support.flags |=
            WIPHY_WOWLAN_SUPPORTS_GTK_REKEY |
            WIPHY_WOWLAN_GTK_REKEY_FAILURE;
        
        priv->wowlan_support.n_patterns = IWLAGN_WOWLAN_MAX_PATTERNS;
        priv->wowlan_support.pattern_min_len =
        IWLAGN_WOWLAN_MIN_PATTERN_LEN;
        priv->wowlan_support.pattern_max_len =
        IWLAGN_WOWLAN_MAX_PATTERN_LEN;
        hw->wiphy->wowlan = &priv->wowlan_support;
    }
#endif
    
    if (iwlwifi_mod_params.power_save)
        hw->wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT;
    else
        hw->wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;
    
    hw->wiphy->max_scan_ssids = PROBE_OPTION_MAX;
    /* we create the 802.11 header and a max-length SSID element */
    hw->wiphy->max_scan_ie_len = capa->max_probe_length - 24 - 34;
    
    /*
     * We don't use all queues: 4 and 9 are unused and any
     * aggregation queue gets mapped down to the AC queue.
     */
    hw->queues = IWLAGN_FIRST_AMPDU_QUEUE;
    
    hw->max_listen_interval = IWL_CONN_MAX_LISTEN_INTERVAL;
    
    if (priv->nvm_data->bands[NL80211_BAND_2GHZ].n_channels)
        priv->hw->wiphy->bands[NL80211_BAND_2GHZ] = &priv->nvm_data->bands[NL80211_BAND_2GHZ];
    if (priv->nvm_data->bands[NL80211_BAND_5GHZ].n_channels)
        priv->hw->wiphy->bands[NL80211_BAND_5GHZ] = &priv->nvm_data->bands[NL80211_BAND_5GHZ];
    
    hw->wiphy->hw_version = priv->trans->hw_id;
    
//    iwl_leds_init(priv);
//    
//    wiphy_ext_feature_set(hw->wiphy, NL80211_EXT_FEATURE_CQM_RSSI_LIST);
//    
//    ret = ieee80211_register_hw(priv->hw);
//    if (ret) {
//        IWL_ERR(priv, "Failed to register hw (error %d)\n", ret);
//        iwl_leds_exit(priv);
//        return ret;
//    }
    priv->mac80211_registered = 1;
    
    return 0;
}


// line 238
int IwlDvmOpMode::__iwl_up(struct iwl_priv *priv)
{
    struct iwl_rxon_context *ctx;
    int ret;
    
    //lockdep_assert_held(&priv->mutex);
    
    if (test_bit(STATUS_EXIT_PENDING, &priv->status)) {
        IWL_WARN(priv, "Exit pending; will not bring the NIC up\n");
        return -EIO;
    }
    
    for_each_context(priv, ctx) {
        ret = iwlagn_alloc_bcast_station(priv, ctx);
        if (ret) {
            // TODO: Implement
            //iwl_dealloc_bcast_stations(priv);
            return ret;
        }
    }
    
    ret = _ops->start_hw(priv->trans, true);
    if (ret) {
        IWL_ERR(priv, "Failed to start HW: %d\n", ret);
        goto error;
    }
    
    ret = iwl_run_init_ucode(priv);
    if (ret) {
        IWL_ERR(priv, "Failed to run INIT ucode: %d\n", ret);
        goto error;
    }
    
    ret = _ops->start_hw(priv->trans, true);
    if (ret) {
        IWL_ERR(priv, "Failed to start HW: %d\n", ret);
        goto error;
    }
    
    ret = iwl_load_ucode_wait_alive(priv, IWL_UCODE_REGULAR);
    if (ret) {
        IWL_ERR(priv, "Failed to start RT ucode: %d\n", ret);
        goto error;
    }

    ret = iwl_alive_start(priv);
    if (ret)
        goto error;
    return 0;
    
error:
    set_bit(STATUS_EXIT_PENDING, &priv->status);
    iwl_down(priv);
    clear_bit(STATUS_EXIT_PENDING, &priv->status);
    
    IWL_ERR(priv, "Unable to initialize device.\n");
    return ret;
}


// line 296
int IwlDvmOpMode::iwlagn_mac_start(struct iwl_priv *priv)
{
    //struct iwl_priv *priv = IWL_MAC80211_GET_DVM(hw);
    int ret;
    
    IWL_DEBUG_MAC80211(priv, "enter\n");
    
    /* we should be verifying the device is ready to be opened */
    IOLockLock(priv->mutex);
    ret = __iwl_up(priv);
    IOLockUnlock(priv->mutex);
    if (ret)
        return ret;
    
    IWL_DEBUG_INFO(priv, "Start UP work done.\n");
    
    /* Now we should be done, and the READY bit should be set. */
    if (!test_bit(STATUS_READY, &priv->status))
        ret = -EIO;
    
    // TODO: Implement
    //iwlagn_led_enable(priv);
    
    priv->is_open = 1;
    IWL_DEBUG_MAC80211(priv, "leave\n");
    return 0;
}

// line 1048
void IwlDvmOpMode::iwl_chswitch_done(struct iwl_priv *priv, bool is_success)
{
    /*
     * MULTI-FIXME
     * See iwlagn_mac_channel_switch.
     */
//    struct iwl_rxon_context *ctx = &priv->contexts[IWL_RXON_CTX_BSS];
    
    if (test_bit(STATUS_EXIT_PENDING, &priv->status))
        return;
    
    if (!test_and_clear_bit(STATUS_CHANNEL_SWITCH_PENDING, &priv->status))
        return;

    // TODO: Implement
//    if (ctx->vif)
//        ieee80211_chswitch_done(ctx->vif, is_success);
}

// line 1637
/* This function both allocates and initializes hw and priv. */
struct ieee80211_hw *IwlDvmOpMode::iwl_alloc_all(void)
{
    struct iwl_priv *priv;
    //struct iwl_op_mode *op_mode;
    /* mac80211 allocates memory for this device instance, including
     *   space for this driver's private structure */
    struct ieee80211_hw *hw;
    
    // This is not kinda correct I guess...
    hw = (struct ieee80211_hw *)IOMalloc(sizeof(ieee80211_hw));
    
    if (!hw)
        goto out;
    
    hw->wiphy = (struct wiphy *)IOMalloc(sizeof(wiphy));
    bzero(hw->wiphy, sizeof(wiphy));
    hw->conf.chandef.chan = (struct ieee80211_channel *)IOMalloc(sizeof(struct ieee80211_channel));
    
    hw->priv = IOMalloc(sizeof(iwl_priv));
    if (!hw->priv)
        goto out;
    bzero(hw->priv, sizeof(iwl_priv));
    
    priv = (struct iwl_priv *)hw->priv;
    priv->hw = hw;
    
//    op_mode = hw->priv;
//    priv = IWL_OP_MODE_GET_DVM(op_mode);
//    priv->hw = hw;
    
out:
    return hw;
}


