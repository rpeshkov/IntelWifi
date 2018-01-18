//
//  IwlDvmOpMode_calib.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 18/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

extern "C" {
#include "dev.h"
}

// line 1098
void IwlDvmOpMode::iwl_reset_run_time_calib(struct iwl_priv *priv)
{
    int i;
    memset(&(priv->sensitivity_data), 0,
           sizeof(struct iwl_sensitivity_data));
    memset(&(priv->chain_noise_data), 0,
           sizeof(struct iwl_chain_noise_data));
    for (i = 0; i < NUM_RX_CHAINS; i++)
        priv->chain_noise_data.delta_gain_code[i] =
        CHAIN_NOISE_DELTA_GAIN_INIT_VAL;
    
    /* Ask for statistics now, the uCode will send notification
     * periodically after association */
    iwl_send_statistics_request(priv, CMD_ASYNC, true);
}

