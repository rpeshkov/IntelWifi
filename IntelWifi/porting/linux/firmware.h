//
//  firmware.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef firmware_h
#define firmware_h

#include "types.h"

struct firmware {
    size_t size;
    const u8 *data;
    
    /* firmware loader private fields */
    void *priv;
};


#endif /* firmware_h */
