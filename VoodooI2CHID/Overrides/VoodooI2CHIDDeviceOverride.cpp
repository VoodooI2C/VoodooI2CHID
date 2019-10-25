//
//  VoodooI2CHIDDeviceOverride.cpp
//  VoodooI2CHID
//
//  Created by Alexandre Daoud on 10/08/2019.
//  Copyright © 2019 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CHIDDeviceOverride.hpp"

#define super VoodooI2CHIDDevice
OSDefineMetaClassAndStructors(VoodooI2CHIDDeviceOverride, VoodooI2CHIDDevice);

IOReturn VoodooI2CHIDDeviceOverride::getHIDDescriptor() {
    IOLog("%s::%s Overriding HID descriptor\n", getName(), name);
    memcpy(&hid_descriptor, &hid_descriptor_override, sizeof(VoodooI2CHIDDeviceHIDDescriptor));

    return parseHIDDescriptor();
}

IOReturn VoodooI2CHIDDeviceOverride::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
    IOLog("%s::%s Overriding report descriptor\n", getName(), name);

    if (!hid_descriptor.wReportDescLength) {
        IOLog("%s::%s Invalid report descriptor size\n", getName(), name);
        return kIOReturnDeviceError;
    }

    IOBufferMemoryDescriptor* report_descriptor = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, hid_descriptor.wReportDescLength);

    if (!report_descriptor) {
        IOLog("%s::%s Could not allocated buffer for report descriptor\n", getName(), name);
        return kIOReturnNoResources;
    }

    report_descriptor->writeBytes(0, report_descriptor_override, hid_descriptor.wReportDescLength);
    *descriptor = report_descriptor;

    return kIOReturnSuccess;
}
