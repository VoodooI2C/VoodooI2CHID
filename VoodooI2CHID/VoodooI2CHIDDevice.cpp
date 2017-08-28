//
//  VoodooI2CHIDDevice.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include <IOKit/hid/IOHIDDevice.h>
#include "VoodooI2CHIDDevice.hpp"
#include "../../../VoodooI2C/VoodooI2C/VoodooI2CController/VoodooI2CControllerNub.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndAbstractStructors(VoodooI2CHIDDevice, IOHIDDevice);

bool VoodooI2CHIDDevice::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    return true;
}

void VoodooI2CHIDDevice::detach(IOService* provider) {
    super::detach(provider);
}

bool VoodooI2CHIDDevice::init(OSDictionary* properties) {
    if (!super::init(properties))
        return false;

    return true;
}

void VoodooI2CHIDDevice::free() {
    super::free();
}

VoodooI2CHIDDevice* VoodooI2CHIDDevice::probe(IOService* provider, SInt32* score) {
    return this;
}

bool VoodooI2CHIDDevice::start(IOService* provider) {
    if (!super::start(provider))
        return false;

    return true;
}

void VoodooI2CHIDDevice::stop(IOService* provider) {
    super::stop(provider);
}
