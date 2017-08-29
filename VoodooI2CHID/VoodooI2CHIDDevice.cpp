//
//  VoodooI2CHIDDevice.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include <IOKit/hid/IOHIDDevice.h>
#include "VoodooI2CHIDDevice.hpp"
#include "../../../VoodooI2C/VoodooI2C/VoodooI2CDevice/VoodooI2CDeviceNub.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(VoodooI2CHIDDevice, IOHIDDevice);

bool VoodooI2CHIDDevice::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    return true;
}

void VoodooI2CHIDDevice::detach(IOService* provider) {
    super::detach(provider);
}

bool VoodooI2CHIDDevice::init(OSDictionary* properties) {
    IOLog("ENTERED!\n");
    if (!super::init(properties))
        return false;

    hid_descriptor = reinterpret_cast<VoodooI2CHIDDeviceHIDDescriptor*>(IOMalloc(sizeof(VoodooI2CHIDDeviceHIDDescriptor)));
    return true;
}

void VoodooI2CHIDDevice::free() {
    IOFree(hid_descriptor, sizeof(VoodooI2CHIDDeviceHIDDescriptor));

    super::free();
}

IOReturn VoodooI2CHIDDevice::getHIDDescriptor() {
    UInt16 length = 2;
    
    VoodooI2CHIDDeviceCommand command;
    command.c.regiser = hid_descriptor_register;
    
    if (api->writeReadI2C(command.data, length, (UInt8*)&hid_descriptor, (UInt16)sizeof(VoodooI2CHIDDeviceHIDDescriptor)) != kIOReturnSuccess) {
        IOLog("%s::%s Request for HID descriptor failed\n", getName(), name);
        return kIOReturnIOError;
    }

    if (hid_descriptor->bcdVersion != 0x0100) {
        IOLog("%s::%s Incorrect BCD version %d\n", getName(), name, hid_descriptor->bcdVersion);
        return kIOReturnInvalid;
    }
    
    if (hid_descriptor->wHIDDescLength != sizeof(VoodooI2CHIDDeviceHIDDescriptor)) {
        IOLog("%s::%s Unexpected size of HID descriptor\n", getName(), name);
        return kIOReturnInvalid;
    }

    OSDictionary* property_array = OSDictionary::withCapacity(1);
    property_array->setObject("HIDDescLength", OSNumber::withNumber(hid_descriptor->wHIDDescLength, 32));
    property_array->setObject("BCDVersion", OSNumber::withNumber(hid_descriptor->bcdVersion, 32));
    property_array->setObject("ReportDescLength", OSNumber::withNumber(hid_descriptor->wReportDescLength, 32));
    property_array->setObject("ReportDescRegister", OSNumber::withNumber(hid_descriptor->wReportDescRegister, 32));
    property_array->setObject("MaxInputLength", OSNumber::withNumber(hid_descriptor->wMaxInputLength, 32));
    property_array->setObject("InputRegister", OSNumber::withNumber(hid_descriptor->wInputRegister, 32));
    property_array->setObject("MaxOutputLength", OSNumber::withNumber(hid_descriptor->wMaxOutputLength, 32));
    property_array->setObject("OutputRegister", OSNumber::withNumber(hid_descriptor->wOutputRegister, 32));
    property_array->setObject("CommandRegister", OSNumber::withNumber(hid_descriptor->wCommandRegister, 32));
    property_array->setObject("DataRegister", OSNumber::withNumber(hid_descriptor->wDataRegister, 32));
    property_array->setObject("VendorID", OSNumber::withNumber(hid_descriptor->wVendorID, 32));
    property_array->setObject("ProductID", OSNumber::withNumber(hid_descriptor->wProductID, 32));
    property_array->setObject("VersionID", OSNumber::withNumber(hid_descriptor->wVersionID, 32));

    setProperty("HIDDescriptor", property_array);

    property_array->release();

    return kIOReturnSuccess;
}

IOReturn VoodooI2CHIDDevice::getHIDDescriptorAddress() {
    UInt32 guid_1 = 0x3CDFF6F7;
    UInt32 guid_2 = 0x45554267;
    UInt32 guid_3 = 0x0AB305AD;
    UInt32 guid_4 = 0xDE38893D;
    
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
    
    acpi_device->evaluateObject("_DSM", &result, params, 4);
    if (!result)
        acpi_device->evaluateObject("XDSM", &result, params, 4);
    if (!result) {
        IOLog("%s::%s Could not find suitable _DSM or XDSM method in ACPI tables\n", getName(), name);
        return kIOReturnNotFound;
    }
    
    OSNumber* number = OSDynamicCast(OSNumber, result);
    if (number){
        setProperty("HIDDescriptorAddress", number);
        hid_descriptor_register = number->unsigned16BitValue();
    }
    
    if (result)
        result->release();
    
    params[0]->release();
    params[1]->release();
    params[2]->release();
    params[3]->release();
    
    if (!number) {
        IOLog("%s::%s HID descriptor register invalid", getName(), name);
        return kIOReturnInvalid;
    }
    
    return kIOReturnSuccess;
}

VoodooI2CHIDDevice* VoodooI2CHIDDevice::probe(IOService* provider, SInt32* score) {
    if (!super::probe(provider, score))
        return NULL;

    name = getMatchedName(provider);
    
    acpi_device = OSDynamicCast(IOACPIPlatformDevice, provider->getProperty("acpi-device"));
    
    api = OSDynamicCast(VoodooI2CDeviceNub, provider);
    
    if (!acpi_device) {
        IOLog("%s::%s Could not get ACPI device\n", getName(), name);
        return NULL;
    }

    if (getHIDDescriptorAddress() != kIOReturnSuccess) {
        IOLog("%s::%s Could not get HID descriptor\n", getName(), name);
        return NULL;
    }
    
    if (getHIDDescriptor() != kIOReturnSuccess) {
        IOLog("%s::%s Could not get HID descriptor", getName(), name);
    }

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

IOReturn VoodooI2CHIDDevice::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
    return kIOReturnNotFound;
}

OSNumber* VoodooI2CHIDDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(hid_descriptor->wVendorID, 16);
}

OSNumber* VoodooI2CHIDDevice::newProductIDNumber() const {
    return OSNumber::withNumber(hid_descriptor->wProductID, 16);
}

OSNumber* VoodooI2CHIDDevice::newVersionNumber() const {
    return OSNumber::withNumber(hid_descriptor->wVersionID, 16);
}

OSString* VoodooI2CHIDDevice::newTransportString() const {
    return OSString::withCString("I2C");
}

OSString* VoodooI2CHIDDevice::newManufacturerString() const {
    return OSString::withCString("Apple");
}

OSNumber* VoodooI2CHIDDevice::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_GD_Mouse, 32);
}

OSNumber* VoodooI2CHIDDevice::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}
