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

#include "IntelIO.hpp"





#define OTP_LOW_IMAGE_SIZE        (2 * 512 * sizeof(UInt16)) /* 2 KB */

class IntelEeprom : public OSObject {
    OSDeclareDefaultStructors(IntelEeprom)
    
public:
    static IntelEeprom* withIO(IntelIO *io);
    bool initWithIO(IntelIO *io);
    void release();
    struct iwl_nvm_data* parse();
    
private:
    const UInt8 *iwl_eeprom_query_addr(UInt32 offset);
    UInt32 eeprom_indirect_address(UInt32 address);
    UInt16 iwl_eeprom_query16(int offset);
    int iwl_eeprom_read_calib(struct iwl_nvm_data *data);
    
    int iwl_eeprom_acquire_semaphore();
    void iwl_eeprom_release_semaphore();
    UInt8* fEeprom;
    
    IntelIO *fIO;
};



#endif /* IntelEeprom_hpp */
