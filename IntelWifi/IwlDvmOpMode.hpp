//
//  IwlDvmOpMode.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IwlDvmOpMode_hpp
#define IwlDvmOpMode_hpp

#include "IwlOpModeOps.h"
#include "IntelEeprom.hpp"

class IwlDvmOpMode : public IwlOpModeOps {
public:
    IwlDvmOpMode(IwlTransOps *ops, IntelIO *io, IntelEeprom *eeprom);
    virtual struct iwl_priv *start(struct iwl_trans *trans,
                                const struct iwl_cfg *cfg,
                                const struct iwl_fw *fw,
                                struct dentry *dbgfs_dir) override;
    virtual void nic_config(struct iwl_priv *priv) override;

    
private:
    struct iwl_priv *iwl_op_mode_dvm_start(struct iwl_trans *trans,
                                        const struct iwl_cfg *cfg,
                                        const struct iwl_fw *fw,
                                        struct dentry *dbgfs_dir); // line 1232
    
    // mac80211.c
    int __iwl_up(struct iwl_priv *priv); // line 238
    int iwlagn_mac_start(struct iwl_priv *priv); // line 296
    
    // ucode.c
    int iwl_init_alive_start(struct iwl_priv *priv);
    void iwl_send_prio_tbl(struct iwl_priv *priv);
    int iwl_send_bt_env(struct iwl_priv *priv, u8 action, u8 type);
    int iwl_load_ucode_wait_alive(struct iwl_priv *priv,
                                  enum iwl_ucode_type ucode_type);
    int iwl_run_init_ucode(struct iwl_priv *priv);
    int iwl_alive_notify(struct iwl_priv *priv);
    void iwl_nic_config(struct iwl_priv *priv);
    
    

    
    IwlTransOps *_ops;
    IntelEeprom *_eeprom;
    IntelIO *_io;
    struct iwl_priv *priv;
};

#endif /* IwlDvmOpMode_hpp */
