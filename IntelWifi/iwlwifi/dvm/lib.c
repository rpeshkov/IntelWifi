//
//  lib.c
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "iwl-agn-hw.h"
#include "iwl-trans.h"
#include "iwl-modparams.h"

#include "dev.h"
#include "agn.h"


void iwlagn_temperature(struct iwl_priv *priv)
{
    //lockdep_assert_held(&priv->statistics.lock);
    
    /* store temperature from correct statistics (in Celsius) */
    priv->temperature = le32_to_cpu(priv->statistics.common.temperature);
    //iwl_tt_handler(priv);
}
