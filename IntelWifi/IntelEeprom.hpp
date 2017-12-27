//
//  IntelEeprom.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 27/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef IntelEeprom_hpp
#define IntelEeprom_hpp

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSMetaClass.h>

#include <sys/errno.h>

#include <IOKit/IOLib.h>

#include "iwlwifi/iwl-eeprom-parse.h"



#define IWL_POLL_INTERVAL 10    /* microseconds */

#define OTP_LOW_IMAGE_SIZE        (2 * 512 * sizeof(UInt16)) /* 2 KB */

class IntelEeprom : public OSObject {
    OSDeclareDefaultStructors(IntelEeprom)
    
public:
    static IntelEeprom* withAddress(volatile void * p);
    bool initWithAddress(volatile void * p);
    UInt8* addr() {
        return fEeprom;
    }
    
    struct iwl_nvm_data* parse();
    
    
private:
    
    int iwl_poll_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout);
    void iwl_write32(UInt32 ofs, UInt32 val);
    UInt32 iwl_read32(UInt32 ofs);
    const UInt8 *iwl_eeprom_query_addr(UInt32 offset);
    UInt32 eeprom_indirect_address(UInt32 address);
    UInt16 iwl_eeprom_query16(int offset);
    int iwl_eeprom_read_calib(struct iwl_nvm_data *data);
    
    volatile void * baseHwAddr;
    UInt8* fEeprom;
};

#endif /* IntelEeprom_hpp */
