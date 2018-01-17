//
//  IwlDvmOpMode_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

IwlDvmOpMode::IwlDvmOpMode(IwlTransOps *ops, IntelIO *io, IntelEeprom *eeprom) {
    _io = io;
    _ops = ops;
    _eeprom = eeprom;
}


struct iwl_priv *IwlDvmOpMode::start(struct iwl_trans *trans, const struct iwl_cfg *cfg, const struct iwl_fw *fw, struct dentry *dbgfs_dir) {
    priv = iwl_op_mode_dvm_start(trans, cfg, fw, dbgfs_dir);
    iwlagn_mac_start(priv);
    return priv;
}

void IwlDvmOpMode::nic_config(struct iwl_priv *priv) {
    
    iwl_nic_config(this->priv);
}

