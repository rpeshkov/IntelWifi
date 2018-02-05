//
//  Configuration.h
//  IntelWifi
//
//  Created by Roman Peshkov on 02/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef Configuration_h
#define Configuration_h

#include <linux/types.h>

#include "iwl-config.h"

struct iwl_cfg* getConfiguration(u16 deviceId, u16 subSystemId);

#endif /* Configuration_h */
