//
//  IwlDvmOpMode_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

#include "dev.h"

IwlDvmOpMode::IwlDvmOpMode(IwlTransOps *ops, IntelIO *io, IntelEeprom *eeprom) {
    _io = io;
    _ops = ops;
    _eeprom = eeprom;
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

