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

    // TODO: Implement
    ret = iwl_alive_start(priv);
//    if (ret)
//        goto error;
    return 0;
    
error:
    set_bit(STATUS_EXIT_PENDING, &priv->status);
    //iwl_down(priv);
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
    //mutex_lock(&priv->mutex);
    IOLockLock(priv->mutex);
    ret = __iwl_up(priv);
    //mutex_unlock(&priv->mutex);
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
    struct iwl_rxon_context *ctx = &priv->contexts[IWL_RXON_CTX_BSS];
    
    if (test_bit(STATUS_EXIT_PENDING, &priv->status))
        return;
    
    if (!test_and_clear_bit(STATUS_CHANNEL_SWITCH_PENDING, &priv->status))
        return;

    // TODO: Implement
//    if (ctx->vif)
//        ieee80211_chswitch_done(ctx->vif, is_success);
}

