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
    // This needs to be investigated further for USB touchpad support,
    // it is currently commented out as it causes issues with input devices
    // failing to wake from sleep and does not work on 10.15 and lower

    /* if (version_major > CATALINA_MAJOR_VERSION) {
        // Update value from hardware so we can rewrite mode when waking from sleep
        digitiser.input_mode->getValue(kIOHIDValueOptionsUpdateElementValues);
        digitiser.input_mode->setValue(INPUT_MODE_TOUCHPAD);
        ready = true;
        return;
    }*/
    
    // TODO: setValue appears to not work on Catalina or older
    VoodooI2CPrecisionTouchpadFeatureReport buffer;
    buffer.reportID = digitiser.input_mode->getReportID();
    buffer.value = INPUT_MODE_TOUCHPAD;
    buffer.reserved = 0x00;
    IOBufferMemoryDescriptor* report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(VoodooI2CPrecisionTouchpadFeatureReport));
    report->writeBytes(0, &buffer, sizeof(VoodooI2CPrecisionTouchpadFeatureReport));

    hid_interface->setReport(report, kIOHIDReportTypeFeature, digitiser.input_mode->getReportID());
    report->release();
    
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
