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
#include "iwl-config.h"

#include "IntelIO.hpp"


class IntelEeprom : public OSObject {
    OSDeclareDefaultStructors(IntelEeprom)
    
public:
    static IntelEeprom* withIO(IntelIO *io, struct iwl_cfg *config, UInt32 hwRev);
    bool initWithIO(IntelIO *io, struct iwl_cfg *config, UInt32 hwRev);
    void release();
    struct iwl_nvm_data* parse();
    
private:
    const UInt8 *iwl_eeprom_query_addr(UInt32 offset);
    UInt32 eeprom_indirect_address(UInt32 address);
    UInt16 iwl_eeprom_query16(int offset);
    int iwl_eeprom_read_calib(struct iwl_nvm_data *data);
    
    int iwl_eeprom_acquire_semaphore();
    void iwl_eeprom_release_semaphore();
    
    int iwl_nvm_is_otp();
    int iwl_eeprom_verify_signature(bool nvm_is_otp);
    int iwl_init_otp_access();
    int iwl_find_otp_image(UInt16 *validblockaddr);
    int iwl_read_otp_word(u16 addr, __le16 *eeprom_data);
    void iwl_set_otp_access_absolute();
    bool iwl_is_otp_empty();
    
    bool read();
    
    UInt8* fEeprom;
    
    IntelIO *fIO;
    struct iwl_cfg *fConfiguration;
    UInt32 fHwRev;
};



#endif /* IntelEeprom_hpp */
