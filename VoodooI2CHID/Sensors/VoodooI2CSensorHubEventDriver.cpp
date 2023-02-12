//
//  VoodooI2CSensorHubEventDriver.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CSensorHubEventDriver.hpp"

#include "VoodooI2CSensor.hpp"
#include "VoodooI2CDeviceOrientationSensor.hpp"
#include "VoodooI2CAccelerometerSensor.hpp"

#define super IOHIDEventService
OSDefineMetaClassAndStructors(VoodooI2CSensorHubEventDriver, IOHIDEventService);

const char* VoodooI2CSensorHubEventDriver::getProductName() {
    VoodooI2CHIDDevice* i2c_hid_device = OSDynamicCast(VoodooI2CHIDDevice, hid_device);
    
    if (i2c_hid_device)
        return i2c_hid_device->name;

    if (OSString* name = getProduct())
        return name->getCStringNoCopy();

    return "HID Sensor";
}

void VoodooI2CSensorHubEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    if (!readyForReports() || report_type != kIOHIDReportTypeInput)
        return;
    
    for (int i = 0; i < sensors->getCount(); i++) {
        VoodooI2CSensor* sensor = OSDynamicCast(VoodooI2CSensor, sensors->getObject(i));
        
        if (!sensor)
            continue;
        
        sensor->handleInterruptReport(timestamp, report, report_type, report_id);
    }
}

bool VoodooI2CSensorHubEventDriver::handleStart(IOService* provider) {
    hid_interface = OSDynamicCast(IOHIDInterface, provider);
    
    if (!hid_interface)
        return false;
    
    hid_device = OSDynamicCast(IOHIDDevice, hid_interface->getParentEntry(gIOServicePlane));
    
    if (!hid_device)
        return false;
    
    name = getProductName();

    IOLog("%s::%s Found HID Sensor Hub\n", getName(), name);
    
    supported_elements = OSDynamicCast(OSArray, hid_device->getProperty(kIOHIDElementKey));
    
    if (!supported_elements)
        return false;
    
    hid_interface->setProperty("VoodooI2CServices Supported", kOSBooleanTrue);
    
    sensors = OSArray::withCapacity(1);
    
    if (!sensors)
        return false;
    
    for (int index=0, count = supported_elements->getCount(); index < count; index++) {
        IOHIDElement* element = NULL;
        
        element = OSDynamicCast(IOHIDElement, supported_elements->getObject(index));
        if (!element)
            continue;
        
        if (element->getUsage() == 0)
            continue;
        
        if (element->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Sensor))
            parseSensorParent(element);
    }
    
    // setSensorHubProperties();
    
    if (!hid_interface->open(this, 0, OSMemberFunctionCast(IOHIDInterface::InterruptReportAction, this, &VoodooI2CSensorHubEventDriver::handleInterruptReport), NULL))
        return false;

    PMinit();
    hid_interface->joinPMtree(this);
    registerPowerDriver(this, VoodooI2CIOPMPowerStates, kVoodooI2CIOPMNumberPowerStates);
    
    return true;
}

void VoodooI2CSensorHubEventDriver::handleStop(IOService* provider) {
    if (sensors) {
        while (sensors->getCount() > 0) {
            VoodooI2CSensor *sensor = OSDynamicCast(VoodooI2CSensor, sensors->getLastObject());
            sensor->stop(this);
            sensor->detach(this);
            sensors->removeObject(sensors->getCount() - 1);
        }
    }
    
    OSSafeReleaseNULL(sensors);
    
    
    PMstop();
    
    super::handleStop(provider);
}

bool VoodooI2CSensorHubEventDriver::willTerminate(IOService* provider, IOOptionBits options) {
    if (hid_interface)
        hid_interface->close(this);
    hid_interface = NULL;
    
    return super::willTerminate(provider, options);
}

IOReturn VoodooI2CSensorHubEventDriver::parseSensorParent(IOHIDElement* parent) {
    OSArray* children = parent->getChildElements();

    VoodooI2CSensor* sensor = NULL;

    for (int i = 0; i < children->getCount(); i++) {
        IOHIDElement* sensor_element = OSDynamicCast(IOHIDElement, children->getObject(i));
        
        if (!sensor_element)
            continue;

        if (sensor_element->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Motion_Accelerometer3D))
            sensor = VoodooI2CAccelerometerSensor::withElement(sensor_element, this);
        
        if (sensor)
            sensors->setObject(sensor);

        OSSafeReleaseNULL(sensor);
    }

    return kIOReturnNoDevice;
}


IOReturn VoodooI2CSensorHubEventDriver::setPowerState(unsigned long whichState, IOService* whatDevice) {
    return kIOPMAckImplied;
}

IOReturn VoodooI2CSensorHubEventDriver::setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, UInt32 reportID) {
    return hid_interface->setReport(report, reportType, reportID);
}

