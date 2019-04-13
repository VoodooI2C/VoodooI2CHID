//
//  VoodooI2CDeviceOrientationSensor.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 26/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CDeviceOrientationSensor.hpp"
#include "VoodooI2CSensorHubEventDriver.hpp"

#define super VoodooI2CSensor
OSDefineMetaClassAndStructors(VoodooI2CDeviceOrientationSensor, VoodooI2CSensor);

void VoodooI2CDeviceOrientationSensor::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    if (!quaternion)
        return;
    
    if (quaternion->getReportID() != report_id)
        return;

    // OSData* data = quaternion->getDataValue();

    // VoodooI2CQuaternion* quaternion = (VoodooI2CQuaternion*)data->getBytesNoCopy();
    
    // IOLog("VoodooI2C Quaternion x: %d, y: %d, z: %d, w: %d\n", quaternion->x, quaternion->y, quaternion->z, quaternion->w);
}

bool VoodooI2CDeviceOrientationSensor::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    IOLog("%s Found a device orientation sensor\n", getName());

    for (int i = 0; i < element->getChildElements()->getCount(); i++) {
        IOHIDElement* child = OSDynamicCast(IOHIDElement, element->getChildElements()->getObject(i));
        
        if (!child)
            continue;
        
        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Orientation_Quaternion)) {
            quaternion = child;
            break;
        }
    }
    
    if (!quaternion)
        return false;

    return true;
}

VoodooI2CSensor* VoodooI2CDeviceOrientationSensor::withElement(IOHIDElement* sensor_element, IOService* event_driver) {
    VoodooI2CSensor* sensor = OSTypeAlloc(VoodooI2CDeviceOrientationSensor);
    
    OSDictionary* dictionary = OSDictionary::withCapacity(1);
    
    dictionary->setObject("HID Element", sensor_element);
    
    sensor->element = sensor_element;
    
    if (!sensor->init(dictionary) ||
        !sensor->attach(event_driver) ||
        !sensor->start(event_driver)) {
        OSSafeReleaseNULL(sensor);
        
        return NULL;
    }
    
    return sensor;
}
