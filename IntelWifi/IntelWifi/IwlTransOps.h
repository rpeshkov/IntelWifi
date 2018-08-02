//
//  IntelWifiTransOps.h
//  IntelWifi
//
//  Created by Roman Peshkov on 10/07/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IntelWifiTransOps_h
#define IntelWifiTransOps_h

#include "TransOps.h"
#include "IntelWifi.hpp"

class IwlTransOps: public TransOps {
    
public:
    IwlTransOps(IntelWifi *iw);
    
    int start_hw(struct iwl_trans *trans, bool low_power) override;
    void op_mode_leave(struct iwl_trans *trans) override;
    void stop_device(struct iwl_trans *trans, bool low_power) override;
    int start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill) override;
    
private:
    IntelWifi *iw;
};

#endif /* IntelWifiTransOps_h */
