/******************************************************************************
 *
 * Copyright(c) 2003 - 2014 Intel Corporation. All rights reserved.
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
//  IwlDvmOpMode_mac80211.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

extern "C" {
#include "iwlwifi/dvm/agn.h"
#include "iwlwifi/dvm/dev.h"
}

#include "IwlDvmOpMode.hpp"

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
int iwlagn_mac_setup_register(struct iwl_priv *priv, const struct iwl_ucode_capabilities *capa)
{
    //int ret;
    struct ieee80211_hw *hw = priv->hw;
    struct iwl_rxon_context *ctx;
    
    hw->rate_control_algorithm = "iwl-agn-rs";
    
    /* Tell mac80211 our characteristics */
    ieee80211_hw_set(hw, SIGNAL_DBM);
    ieee80211_hw_set(hw, AMPDU_AGGREGATION);
    ieee80211_hw_set(hw, NEED_DTIM_BEFORE_ASSOC);
    ieee80211_hw_set(hw, SPECTRUM_MGMT);
    ieee80211_hw_set(hw, REPORTS_TX_ACK_STATUS);
    ieee80211_hw_set(hw, QUEUE_CONTROL);
    ieee80211_hw_set(hw, SUPPORTS_PS);
    ieee80211_hw_set(hw, SUPPORTS_DYNAMIC_PS);
    ieee80211_hw_set(hw, SUPPORT_FAST_XMIT);
    ieee80211_hw_set(hw, WANT_MONITOR_VIF);
    
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
        hw->wiphy->features |= NL80211_FEATURE_DYNAMIC_SMPS | NL80211_FEATURE_STATIC_SMPS;
    
    /*
     * Enable 11w if advertised by firmware and software crypto
     * is not enabled (as the firmware will interpret some mgmt
     * packets, so enabling it with software crypto isn't safe)
     */
    if (priv->fw->ucode_capa.flags & IWL_UCODE_TLV_FLAGS_MFP && !iwlwifi_mod_params.swcrypto)
        ieee80211_hw_set(hw, MFP_CAPABLE);
    
    hw->sta_data_size = sizeof(struct iwl_station_priv);
    hw->vif_data_size = sizeof(struct iwl_vif_priv);
    
    for_each_context(priv, ctx) {
        hw->wiphy->interface_modes |= ctx->interface_modes;
        hw->wiphy->interface_modes |= ctx->exclusive_interface_modes;
    }
    
    BUILD_BUG_ON(NUM_IWL_RXON_CTX != 2);
    
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
            iwl_dealloc_bcast_stations(priv);
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

// line 323
void IwlDvmOpMode::iwlagn_mac_stop(struct iwl_priv *priv)
{
    IWL_DEBUG_MAC80211(priv, "enter\n");
    
    if (!priv->is_open)
        return;
    
    priv->is_open = 0;
    
    IOLockLock(priv->mutex);
    iwl_down(priv);
    IOLockUnlock(priv->mutex);
    
    // TODO: Implement
//    iwl_cancel_deferred_work(priv);
    
//    flush_workqueue(priv->workqueue);
    
    IWL_DEBUG_MAC80211(priv, "leave\n");

}

// line 964
void IwlDvmOpMode::iwlagn_mac_channel_switch(struct iwl_priv *priv, struct ieee80211_vif *vif,
                                             struct ieee80211_channel_switch *ch_switch)
{
    //struct iwl_priv *priv = IWL_MAC80211_GET_DVM(hw);
    //struct ieee80211_conf *conf = &hw->conf;
    struct ieee80211_channel *channel = ch_switch->chandef.chan;
    struct iwl_ht_config *ht_conf = &priv->current_ht_config;
    /*
     * MULTI-FIXME
     * When we add support for multiple interfaces, we need to
     * revisit this. The channel switch command in the device
     * only affects the BSS context, but what does that really
     * mean? And what if we get a CSA on the second interface?
     * This needs a lot of work.
     */
    struct iwl_rxon_context *ctx = &priv->contexts[IWL_RXON_CTX_BSS];
    u16 ch;
    
    IWL_DEBUG_MAC80211(priv, "enter\n");
    
    IOLockLock(priv->mutex);
    
    if (iwl_is_rfkill(priv))
        goto out;
    
    if (test_bit(STATUS_EXIT_PENDING, &priv->status) ||
        test_bit(STATUS_SCANNING, &priv->status) ||
        test_bit(STATUS_CHANNEL_SWITCH_PENDING, &priv->status))
        goto out;
    
    if (!iwl_is_associated_ctx(ctx))
        goto out;
    
    if (!priv->lib->set_channel_switch)
        goto out;
    
    ch = channel->hw_value;
    if (le16_to_cpu(ctx->active.channel) == ch)
        goto out;
    
    priv->current_ht_config.smps = priv->hw->conf.smps_mode;
    
    /* Configure HT40 channels */
    switch (cfg80211_get_chandef_type(&ch_switch->chandef)) {
        case NL80211_CHAN_NO_HT:
        case NL80211_CHAN_HT20:
            ctx->ht.is_40mhz = false;
            ctx->ht.extension_chan_offset = IEEE80211_HT_PARAM_CHA_SEC_NONE;
            break;
        case NL80211_CHAN_HT40MINUS:
            ctx->ht.extension_chan_offset = IEEE80211_HT_PARAM_CHA_SEC_BELOW;
            ctx->ht.is_40mhz = true;
            break;
        case NL80211_CHAN_HT40PLUS:
            ctx->ht.extension_chan_offset = IEEE80211_HT_PARAM_CHA_SEC_ABOVE;
            ctx->ht.is_40mhz = true;
            break;
    }
    
    if ((le16_to_cpu(ctx->staging.channel) != ch))
        ctx->staging.flags = 0;
    
    iwl_set_rxon_channel(priv, channel, ctx);
    iwl_set_rxon_ht(priv, ht_conf);
    iwl_set_flags_for_band(priv, ctx, channel->band, ctx->vif);
    
    /*
     * at this point, staging_rxon has the
     * configuration for channel switch
     */
    set_bit(STATUS_CHANNEL_SWITCH_PENDING, &priv->status);
    priv->switch_channel = cpu_to_le16(ch);
    if (priv->lib->set_channel_switch(priv, ch_switch)) {
        clear_bit(STATUS_CHANNEL_SWITCH_PENDING, &priv->status);
        priv->switch_channel = 0;
        //ieee80211_chswitch_done(ctx->vif, false);
    }
    priv->hw->conf.chandef = ch_switch->chandef;
    
out:
    IOLockUnlock(priv->mutex);
    IWL_DEBUG_MAC80211(priv, "leave\n");
}


// line 1048
void iwl_chswitch_done(struct iwl_priv *priv, bool is_success)
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

// line 1242
static int iwl_set_mode(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    iwl_connection_init_rx_config(priv, ctx);
    
    iwlagn_set_rxon_chain(priv, ctx);
    
    return iwlagn_commit_rxon(priv, ctx);
}


// line 1251
int IwlDvmOpMode::iwl_setup_interface(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    struct ieee80211_vif *vif = ctx->vif;
    int err, ac;
    
    //lockdep_assert_held(&priv->mutex);
    
    /*
     * This variable will be correct only when there's just
     * a single context, but all code using it is for hardware
     * that supports only one context.
     */
    priv->iw_mode = vif->type;
    
    ctx->is_active = true;
    
    err = iwl_set_mode(priv, ctx);
    if (err) {
        if (!ctx->always_active)
            ctx->is_active = false;
        return err;
    }
    
    if (priv->lib->bt_params && priv->lib->bt_params->advanced_bt_coexist && vif->type == NL80211_IFTYPE_ADHOC) {
        /*
         * pretend to have high BT traffic as long as we
         * are operating in IBSS mode, as this will cause
         * the rate scaling etc. to behave as intended.
         */
        priv->bt_traffic_load = IWL_BT_COEX_TRAFFIC_LOAD_HIGH;
    }
    
    /* set up queue mappings */
    for (ac = 0; ac < IEEE80211_NUM_ACS; ac++)
        vif->hw_queue[ac] = ctx->ac_to_queue[ac];
    
    if (vif->type == NL80211_IFTYPE_AP)
        vif->cab_queue = ctx->mcast_queue;
    else
        vif->cab_queue = IEEE80211_INVAL_HW_QUEUE;
    
    return 0;
}


// line 1297
int IwlDvmOpMode::iwlagn_mac_add_interface(struct iwl_priv *priv, struct ieee80211_vif *vif)
{
//    struct iwl_priv *priv = IWL_MAC80211_GET_DVM(hw);
    struct iwl_vif_priv *vif_priv = (struct iwl_vif_priv *)vif->drv_priv;
    struct iwl_rxon_context *tmp, *ctx = NULL;
    int err;
    enum nl80211_iftype viftype = ieee80211_vif_type_p2p(vif);
    bool reset = false;
    
    IWL_DEBUG_MAC80211(priv, "iwlagn_mac_add_interface enter: type %d, addr", viftype);
    
    //mutex_lock(&priv->mutex);
    IOLockLock(priv->mutex);
    
    if (!iwl_is_ready_rf(priv)) {
        IWL_WARN(priv, "Try to add interface when device not ready\n");
        err = -EINVAL;
        goto out;
    }
    
    for_each_context(priv, tmp) {
        u32 possible_modes = tmp->interface_modes | tmp->exclusive_interface_modes;
        
        IWL_DEBUG_INFO(priv, "POSSIBLE MODES: %d; BIT(viftype): %lu", (unsigned int)possible_modes, BIT(viftype));
        
        if (tmp->vif) {
            /* On reset we need to add the same interface again */
            if (tmp->vif == vif) {
                reset = true;
                ctx = tmp;
                break;
            }
            
            /* check if this busy context is exclusive */
            if (tmp->exclusive_interface_modes & BIT(tmp->vif->type)) {
                err = -EINVAL;
                goto out;
            }
            continue;
        }
        
        if (!(possible_modes & BIT(viftype)))
            continue;
        
        /* have maybe usable context w/o interface */
        ctx = tmp;
        break;
    }
    
    if (!ctx) {
        err = -EOPNOTSUPP;
        goto out;
    }
    
    vif_priv->ctx = ctx;
    ctx->vif = vif;
    
    /*
     * In SNIFFER device type, the firmware reports the FCS to
     * the host, rather than snipping it off. Unfortunately,
     * mac80211 doesn't (yet) provide a per-packet flag for
     * this, so that we have to set the hardware flag based
     * on the interfaces added. As the monitor interface can
     * only be present by itself, and will be removed before
     * other interfaces are added, this is safe.
     */
    if (vif->type == NL80211_IFTYPE_MONITOR)
        ieee80211_hw_set(priv->hw, RX_INCLUDES_FCS);
    else
        clear_bit(IEEE80211_HW_RX_INCLUDES_FCS, priv->hw->flags);
    
    err = iwl_setup_interface(priv, ctx);
    if (!err || reset)
        goto out;
    
    ctx->vif = NULL;
    priv->iw_mode = NL80211_IFTYPE_STATION;
out:
    //mutex_unlock(&priv->mutex);
    IOLockUnlock(priv->mutex);
    
    IWL_DEBUG_MAC80211(priv, "iwlagn_mac_add_interface leave\n");
    return err;
}


// line 1637
/* This function both allocates and initializes hw and priv. */
struct ieee80211_hw *iwl_alloc_all(void)
{
    struct iwl_priv *priv;
    //struct iwl_op_mode *op_mode;
    /* mac80211 allocates memory for this device instance, including
     *   space for this driver's private structure */
    struct ieee80211_hw *hw;
    
    // This is not kinda correct I guess...
    hw = (struct ieee80211_hw *)iwh_malloc(sizeof(ieee80211_hw));
    if (!hw)
        goto out;
    
    hw->wiphy = (struct wiphy *)iwh_zalloc(sizeof(wiphy));
    if (!hw->wiphy)
        goto out;
    
    hw->conf.chandef.chan = (struct ieee80211_channel *)iwh_zalloc(sizeof(struct ieee80211_channel));
    
    hw->priv = iwh_zalloc(sizeof(iwl_priv));
    if (!hw->priv)
        goto out;
    
    priv = (struct iwl_priv *)hw->priv;
    priv->hw = hw;
    
//    op_mode = hw->priv;
//    priv = IWL_OP_MODE_GET_DVM(op_mode);
//    priv->hw = hw;
    
out:
    return hw;
}


