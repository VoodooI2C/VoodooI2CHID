//
//  VoodooI2CPrecisionTouchpadHIDEventDriver.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 21/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CPrecisionTouchpadHIDEventDriver.hpp"

#define super VoodooI2CMultitouchHIDEventDriver
OSDefineMetaClassAndStructors(VoodooI2CPrecisionTouchpadHIDEventDriver, VoodooI2CMultitouchHIDEventDriver);

void VoodooI2CPrecisionTouchpadHIDEventDriver::enterPrecisionTouchpadMode() {
    int retry = 0;

    // Wait until input_mode becomes available
    while (!digitiser.input_mode && retry < kMaxRetries) {
        IOSleep(50);
        retry++;
    }
    
    // Still not ready -> give up
    if (!digitiser.input_mode) {
        IOLog("VoodooI2CHID: retry limit reached\n");
        return;
    }
    
    UInt8 inputMode[] = { INPUT_MODE_TOUCHPAD };
    
    // Use setDataValue as it does not check for duplicate writes
    OSData *value = OSData::withBytes(inputMode, sizeof(inputMode));
    
    if (!value) {
        IOLog("VoodooI2CHID: failed to allocate memory for input mode setDataValue\n");
        return;
    }
    
    digitiser.input_mode->setDataValue(value);
    OSSafeReleaseNULL(value);
    ready = true;
}

void VoodooI2CPrecisionTouchpadHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) {
    if (!ready)
        return;

    super::handleInterruptReport(timestamp, report, report_type, report_id);
}

bool VoodooI2CPrecisionTouchpadHIDEventDriver::handleStart(IOService* provider) {
    if (!super::handleStart(provider))
        return false;

    if (!digitiser.input_mode)
        return false;

    IOLog("%s::%s Putting device into Precision Touchpad Mode\n", getName(), name);

    ready = false;
    
    enterPrecisionTouchpadMode();

    return true;
}

IOReturn VoodooI2CPrecisionTouchpadHIDEventDriver::parseElements(UInt32) {
    return super::parseElements(kHIDUsage_Dig_TouchPad);
}

IOReturn VoodooI2CPrecisionTouchpadHIDEventDriver::setPowerState(unsigned long whichState, IOService* whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (!whichState) {
        if (awake)
            awake = false;
        ready = false;
    } else {
        if (!awake) {
            ready = false;
            IOSleep(10);
            enterPrecisionTouchpadMode();

            awake = true;
        }
    }
    return kIOPMAckImplied;
}
