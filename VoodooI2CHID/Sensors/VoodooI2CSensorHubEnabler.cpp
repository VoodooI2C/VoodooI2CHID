//
//  VoodooI2CSensorHubEnabler.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CSensorHubEnabler.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooI2CSensorHubEnabler, IOService);

bool VoodooI2CSensorHubEnabler::start(IOService* parent) {
    if (!super::start(parent))
        return false;
    
    provider = OSDynamicCast(IOACPIPlatformDevice, parent);
    
    if (!provider)
        return false;
    
    name = getMatchedName(provider);

    IOLog("%s::%s Found Intel ACPI Sensor Hub Enabler\n", getName(), name);
    
    if (enableHub() != kIOReturnSuccess)
        return false;
    
    registerService();
    
    return true;
}

IOReturn VoodooI2CSensorHubEnabler::enableHub() {
    UInt32 guid_1 = 0x03C868D5;
    UInt32 guid_2 = 0x563F42A8;
    UInt32 guid_3 = 0x9F579A18;
    UInt32 guid_4 = 0xD949B7CB;
    
    OSObject *result = NULL;
    OSObject *params[4];
    char buffer[16];
    
    memcpy(buffer, &guid_1, 4);
    memcpy(buffer + 4, &guid_2, 4);
    memcpy(buffer + 8, &guid_3, 4);
    memcpy(buffer + 12, &guid_4, 4);
    
    
    params[0] = OSData::withBytes(buffer, 16);
    params[1] = OSNumber::withNumber(0x1, 8);
    params[2] = OSNumber::withNumber(0x1, 8);
    params[3] = OSNumber::withNumber((unsigned long long)0x0, 8);
    
    provider->evaluateObject("_DSM", &result, params, 4);
    if (!result)
        provider->evaluateObject("XDSM", &result, params, 4);
    if (!result) {
        IOLog("%s::%s Could not find suitable _DSM or XDSM method in ACPI tables\n", getName(), name);
        return kIOReturnNotFound;
    }
    
    IOLog("%s::%s Enabled Sensor Hub\n", getName(), name);
    
    if (result)
        result->release();
    
    params[0]->release();
    params[1]->release();
    params[2]->release();
    params[3]->release();
    
    return kIOReturnSuccess;
}
