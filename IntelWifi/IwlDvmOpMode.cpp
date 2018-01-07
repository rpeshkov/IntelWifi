//
//  IwlDvmOpMode_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

struct iwl_priv *IwlDvmOpMode::start(struct iwl_trans *trans, const struct iwl_cfg *cfg, const struct iwl_fw *fw, struct dentry *dbgfs_dir) {
    struct iwl_priv *priv = iwl_op_mode_dvm_start(trans, cfg, fw, dbgfs_dir);
    iwlagn_mac_start(priv);
    return priv;
}
