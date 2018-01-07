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
    IwlDvmOpMode(IwlTransOps *ops, IntelEeprom *eeprom);
    virtual IwlOpModeOps *start(struct iwl_trans *trans,
                                const struct iwl_cfg *cfg,
                                const struct iwl_fw *fw,
                                struct dentry *dbgfs_dir) override;

    
private:
    IwlOpModeOps *iwl_op_mode_dvm_start(struct iwl_trans *trans,
                                        const struct iwl_cfg *cfg,
                                        const struct iwl_fw *fw,
                                        struct dentry *dbgfs_dir); // line 1232

    
    IwlTransOps *_ops;
    IntelEeprom *_eeprom;
};

#endif /* IwlDvmOpMode_hpp */
