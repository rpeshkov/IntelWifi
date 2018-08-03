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

IwlDvmOpMode::IwlDvmOpMode(TransOps *ops) {
    _ops = ops;
}


struct ieee80211_hw *IwlDvmOpMode::start(struct iwl_trans *trans, const struct iwl_cfg *cfg, const struct iwl_fw *fw) {
    priv = iwl_op_mode_dvm_start(trans, cfg, fw);
//    iwlagn_mac_start(priv);
//    struct ieee80211_vif *vif = (struct ieee80211_vif *)iwh_malloc(sizeof(struct ieee80211_vif) + sizeof(struct iwl_vif_priv));
//    memcpy(vif->addr, &this->priv->hw->wiphy->addresses[0], ETH_ALEN);
//    vif->type = NL80211_IFTYPE_STATION;
//    IOSleep(1000);
//    int ret = iwlagn_mac_add_interface(this->priv, vif);
//    IWL_DEBUG_INFO(this->priv->trans, "ADD_INTERFACE: %d", ret);
    
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

IOReturn IwlDvmOpMode::getPOWER(IO80211Interface *intf, apple80211_power_data *power_data) {
    power_data->version = APPLE80211_VERSION;
    power_data->num_radios = 1;
    power_data->power_state[0] = priv->is_open == 0 ? APPLE80211_POWER_OFF : APPLE80211_POWER_ON;
    return kIOReturnSuccess;
}
IOReturn IwlDvmOpMode::setPOWER(IO80211Interface *intf, apple80211_power_data *power_data) {
    
    if (!power_data || power_data->num_radios <= 0) {
        return kIOReturnError;
    }
    
    apple80211_power_state power_state = (apple80211_power_state)power_data->power_state[0];
    
    switch (power_state) {
        case APPLE80211_POWER_OFF:
            IWL_WARN(this->priv->trans, "Power off not yet implemented");
        case APPLE80211_POWER_ON:
            iwlagn_mac_start(priv);
        default:
            IWL_WARN(this->priv->trans, "Don't know what to do with this state: %d", power_state);
    }
    
    return kIOReturnSuccess;
}

//void IwlDvmOpMode::add_interface(struct ieee80211_vif *vif) {
////    struct ieee80211_channel_switch *chsw = (struct ieee80211_channel_switch *)iwh_malloc(sizeof(struct ieee80211_channel_switch));
////    chsw->count = 1;
////    chsw->timestamp = jiffies;
////    chsw->block_tx = true;
////    chsw->chandef.chan = &this->priv->nvm_data->channels[0];
////    chsw->chandef.width = NL80211_CHAN_WIDTH_20;
////    iwlagn_mac_channel_switch(this->priv, vif, chsw);
//
////    int ret = iwlagn_mac_add_interface(this->priv, vif);
////    IWL_DEBUG_INFO(this->priv->trans, "ADD_INTERFACE: %d", ret);
//}
//
//void IwlDvmOpMode::channel_switch(struct iwl_priv *priv, struct ieee80211_vif *vif, struct ieee80211_channel_switch *chsw) {
//    iwlagn_mac_channel_switch(this->priv, vif, chsw);
//}
//
