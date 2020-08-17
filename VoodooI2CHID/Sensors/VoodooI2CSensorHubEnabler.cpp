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
    
    UInt32 result;
    if (enableHub(&result) != kIOReturnSuccess || !result)
        return false;

    IOLog("%s::%s Enabled Sensor Hub\n", getName(), name);

    registerService();
    
    return true;
}

IOReturn VoodooI2CSensorHubEnabler::enableHub(UInt32 *result) {
    IOReturn ret;
    uuid_t guid;
    uuid_parse(SHAD_DSM_GUID, guid);

    // convert to mixed-endian
    *(reinterpret_cast<uint32_t *>(guid)) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(guid)));
    *(reinterpret_cast<uint16_t *>(guid) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 2));
    *(reinterpret_cast<uint16_t *>(guid) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 3));

    OSObject *params[] = {
        OSData::withBytes(guid, 16),
        OSNumber::withNumber(SHAD_DSM_REVISION, 8),
        OSNumber::withNumber(SHAD_ON_INDEX, 8),
        OSArray::withCapacity(1),
    };

    ret = provider->evaluateInteger("_DSM", result, params, 4);
    if (ret != kIOReturnSuccess)
        ret = provider->evaluateInteger("XDSM", result, params, 4);

    params[0]->release();
    params[1]->release();
    params[2]->release();
    params[3]->release();

    return ret;
}
