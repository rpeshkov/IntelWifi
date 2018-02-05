//
//  IwlDvmOpMode_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

#include "dev.h"
#include "agn.h"

IwlDvmOpMode::IwlDvmOpMode(IwlTransOps *ops) {
    _ops = ops;
    mutex = IOLockAlloc();
}


struct ieee80211_hw *IwlDvmOpMode::start(struct iwl_trans *trans, const struct iwl_cfg *cfg, const struct iwl_fw *fw, struct dentry *dbgfs_dir) {
    //IOLockLock(mutex);
    priv = iwl_op_mode_dvm_start(trans, cfg, fw, dbgfs_dir);
    iwlagn_mac_start(priv);
    struct ieee80211_vif *vif = (struct ieee80211_vif *)IOMalloc(sizeof(struct ieee80211_vif) + sizeof(struct iwl_vif_priv));
    memcpy(vif->addr, &this->priv->hw->wiphy->addresses[0], ETH_ALEN);
    vif->type = NL80211_IFTYPE_STATION;
    IOSleep(1000);
    int ret = iwlagn_mac_add_interface(this->priv, vif);
    IWL_DEBUG_INFO(this->priv->trans, "ADD_INTERFACE: %d", ret);
    
    //IOLockUnlock(mutex);
    return priv->hw;
}

void IwlDvmOpMode::nic_config(struct iwl_priv *priv) {
    iwl_nic_config(this->priv);
}

void IwlDvmOpMode::stop(struct iwl_priv *priv) {
    iwl_op_mode_dvm_stop(priv);
}

void IwlDvmOpMode::rx(struct iwl_priv *priv, struct napi_struct *napi, struct iwl_rx_cmd_buffer *rxb) {
    iwl_rx_dispatch(this->priv, napi, rxb);
}

void IwlDvmOpMode::scan() {
    this->priv->scan_request = (struct cfg80211_scan_request *)IOMalloc(sizeof(struct cfg80211_scan_request) + sizeof(void*) * 7);
    bzero(this->priv->scan_request, sizeof(struct cfg80211_scan_request));
    this->priv->scan_request->n_channels = 7;
    for (int i = 0; i < this->priv->scan_request->n_channels; ++i) {
        this->priv->scan_request->channels[i] = &priv->nvm_data->channels[i];
    }
    
    int ret = iwl_scan_initiate(this->priv, NULL, IWL_SCAN_NORMAL, NL80211_BAND_2GHZ);
    
    if (ret) {
        IWL_DEBUG_SCAN(this->priv, "Scan failed with status: %d", ret);
    }
}

void IwlDvmOpMode::add_interface(struct ieee80211_vif *vif) {
    DebugLog("ADD INTERFACE");
//    IOLockLock(mutex);
    
    
//    struct ieee80211_channel_switch *chsw = (struct ieee80211_channel_switch *)IOMalloc(sizeof(struct ieee80211_channel_switch));
//    chsw->count = 1;
//    chsw->timestamp = jiffies;
//    chsw->block_tx = true;
//    chsw->chandef.chan = &this->priv->nvm_data->channels[0];
//    chsw->chandef.width = NL80211_CHAN_WIDTH_20;
//    iwlagn_mac_channel_switch(this->priv, vif, chsw);
    
//    int ret = iwlagn_mac_add_interface(this->priv, vif);
//    IWL_DEBUG_INFO(this->priv->trans, "ADD_INTERFACE: %d", ret);
    
    
//    IOLockUnlock(mutex);
}

void IwlDvmOpMode::channel_switch(struct iwl_priv *priv, struct ieee80211_vif *vif, struct ieee80211_channel_switch *chsw) {
    iwlagn_mac_channel_switch(this->priv, vif, chsw);
}

