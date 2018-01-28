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


