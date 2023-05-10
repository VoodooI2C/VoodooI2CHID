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
    UInt8 inputMode[] = { INPUT_MODE_TOUCHPAD };
    
    // Use setDataValue as it does not check for duplicate writes
    OSData *value = OSData::withBytes(inputMode, sizeof(inputMode));
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
    } else {
        if (!awake) {
            IOSleep(10);
            enterPrecisionTouchpadMode();

            awake = true;
        }
    }
    return kIOPMAckImplied;
}
