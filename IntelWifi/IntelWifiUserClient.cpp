//
//  IntelWifiUserClient.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 31/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IntelWifiUserClient.hpp"

#define super IOUserClient

OSDefineMetaClassAndStructors(IntelWifiUserClient, IOUserClient)

const IOExternalMethodDispatch IntelWifiUserClient::sMethods[kNumberOfMethods] = {
    { //kMyUserClientOpen
        (IOExternalMethodAction) &IntelWifiUserClient::openUserClient,
        0,
        0,
        0,
        0
    }
};

bool IntelWifiUserClient::start(IOService *provider) {
    fProvider = OSDynamicCast(IntelWifi, provider);
    
    if (fProvider == NULL) {
        return false;
    }
    
    return super::start(provider);
}


void IntelWifiUserClient::stop(IOService *provider) {
    super::stop(provider);
}


IOReturn IntelWifiUserClient::externalMethod(uint32_t selector,
                                             IOExternalMethodArguments *arguments,
                                             IOExternalMethodDispatch *dispatch,
                                             OSObject *target,
                                             void *reference) {
 
    if (selector < (uint32_t) kNumberOfMethods) {
        dispatch = (IOExternalMethodDispatch *) &sMethods[selector];
        
        if (!target) {
            target = this;
        }
    }
    
    return super::externalMethod(selector, arguments, dispatch, target, reference);
    
}

IOReturn IntelWifiUserClient::openUserClient(IntelWifiUserClient *target,
                                             void *reference,
                                             IOExternalMethodArguments *arguments) {
    return target->openUserClientImpl();
}

IOReturn IntelWifiUserClient::openUserClientImpl() {
    this->fProvider->opmode->scan();
    return kIOReturnSuccess;
}




