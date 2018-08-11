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

IOReturn IwlDvmOpMode::getCARD_CAPABILITIES(IO80211Interface *interface,
                                            struct apple80211_capability_data *cd) {
    cd->version = APPLE80211_VERSION;
    cd->capabilities[0] = 0xab;
    cd->capabilities[1] = 0x7e;
    return kIOReturnSuccess;
}

IOReturn IwlDvmOpMode::getPHY_MODE(IO80211Interface *interface, struct apple80211_phymode_data *pd) {
    pd->version = APPLE80211_VERSION;
    
    struct iwl_nvm_data *data = priv->nvm_data;
    if (data->sku_cap_band_24GHz_enable) {
        pd->phy_mode |= APPLE80211_MODE_11B | APPLE80211_MODE_11G;
    }
    if (data->sku_cap_band_52GHz_enable) {
        pd->phy_mode |= APPLE80211_MODE_11A;
    }
    if (data->sku_cap_11n_enable) {
        pd->phy_mode |= APPLE80211_MODE_11N;
    }
    pd->active_phy_mode = APPLE80211_MODE_AUTO;
    return kIOReturnSuccess;
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
                iwlagn_mac_stop(priv);
                break;
            case APPLE80211_POWER_ON:
                iwlagn_mac_start(priv);
                break;
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
