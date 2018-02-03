//
//  IwlDvmOpMode_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

#include "dev.h"

IwlDvmOpMode::IwlDvmOpMode(IwlTransOps *ops) {
    _ops = ops;
}


struct ieee80211_hw *IwlDvmOpMode::start(struct iwl_trans *trans, const struct iwl_cfg *cfg, const struct iwl_fw *fw, struct dentry *dbgfs_dir) {
    priv = iwl_op_mode_dvm_start(trans, cfg, fw, dbgfs_dir);
    iwlagn_mac_start(priv);
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
//    struct cfg80211_ssid *ssids;
//    int n_ssids;
//    u32 n_channels;
//    enum nl80211_bss_scan_width scan_width;
//    const u8 *ie;
//    size_t ie_len;
//    u16 duration;
//    bool duration_mandatory;
//    u32 flags;
//
//    u32 rates[NUM_NL80211_BANDS];
//
//    struct wireless_dev *wdev;
//
//    u8 mac_addr[ETH_ALEN] __aligned(2);
//    u8 mac_addr_mask[ETH_ALEN] __aligned(2);
//    u8 bssid[ETH_ALEN] __aligned(2);
//
//    /* internal */
//    struct wiphy *wiphy;
//    unsigned long scan_start;
//    struct cfg80211_scan_info info;
//    bool notified;
//    bool no_cck;
//
//    /* keep last */
//    struct ieee80211_channel *channels[0];
    
    this->priv->scan_request = (struct cfg80211_scan_request *)IOMalloc(sizeof(struct cfg80211_scan_request) + sizeof(void*) * 7);
    bzero(this->priv->scan_request, sizeof(struct cfg80211_scan_request));
    this->priv->scan_request->n_channels = 7;
    for (int i = 0; i < this->priv->scan_request->n_channels; ++i) {
        this->priv->scan_request->channels[i] = &priv->nvm_data->channels[i];
    }
    
    //memcpy(this->priv->scan_request->channels[0], &priv->nvm_data->channels[0], sizeof(struct ieee80211_channel));
    
//    bzero(this->priv->scan_request->channels[0], sizeof(struct ieee80211_channel));
//    this->priv->scan_request->channels[0]->band = NL80211_BAND_2GHZ;
    
    iwl_scan_initiate(this->priv, NULL, IWL_SCAN_NORMAL, NL80211_BAND_2GHZ);
}

