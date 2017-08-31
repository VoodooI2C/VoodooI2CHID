//
//  VoodooI2CPrecisionTouchpadDevice.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 31/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CPrecisionTouchpadDevice.hpp"

#define super VoodooI2CHIDDevice
OSDefineMetaClassAndStructors(VoodooI2CPrecisionTouchpadDevice, VoodooI2CHIDDevice);

OSNumber* VoodooI2CPrecisionTouchpadDevice::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_Dig_TouchPad, 32);
}

OSNumber* VoodooI2CPrecisionTouchpadDevice::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_Digitizer, 32);
}

bool VoodooI2CPrecisionTouchpadDevice::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    IOLog("%s::%s Putting device into Precision Touchpad mode\n", getName(), name);

    VoodooI2CPrecisionTouchpadInputModeFeatureReport buffer;
    buffer.mode = INPUT_MODE_TOUCHPAD;
    buffer.reserved = 0x00;
    
    IOBufferMemoryDescriptor* report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(VoodooI2CPrecisionTouchpadInputModeFeatureReport));
    report->writeBytes(0, &buffer, sizeof(VoodooI2CPrecisionTouchpadInputModeFeatureReport));
    
    setReport(report, kIOHIDReportTypeFeature, INPUT_CONFIG_REPORT_ID);

    return true;
}
