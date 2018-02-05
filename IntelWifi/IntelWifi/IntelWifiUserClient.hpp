//
//  IntelWifiUserClient.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 31/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IntelWifiUserClient_hpp
#define IntelWifiUserClient_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>

#include "IntelWifi.hpp"

#include "kext_user_shared.h"


class IntelWifiUserClient : public IOUserClient {
    OSDeclareDefaultStructors(IntelWifiUserClient)
    
protected:
    IntelWifi *fProvider;
    
    static const IOExternalMethodDispatch sMethods[kNumberOfMethods];
    
public:
    virtual void stop(IOService* provider);
    virtual bool start(IOService* provider);
    
protected:
    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments *arguments,
                                    IOExternalMethodDispatch *dispatch, OSObject *target, void *reference);
    
    static IOReturn scan(IntelWifiUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    IOReturn scanImpl();
};


#endif /* IntelWifiUserClient_hpp */
