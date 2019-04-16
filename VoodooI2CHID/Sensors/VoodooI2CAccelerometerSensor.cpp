//
//  VoodooI2CAccelerometerSensor.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 28/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CAccelerometerSensor.hpp"
#include "VoodooI2CSensorHubEventDriver.hpp"

#define super VoodooI2CSensor
OSDefineMetaClassAndStructors(VoodooI2CAccelerometerSensor, VoodooI2CSensor);

IOFramebuffer* VoodooI2CAccelerometerSensor::getFramebuffer() {
    IODisplay* display = NULL;
    IOFramebuffer* framebuffer = NULL;
    
    OSDictionary *match = serviceMatching("IODisplay");
    OSIterator *iterator = getMatchingServices(match);
    
    if (iterator) {
        display = OSDynamicCast(IODisplay, iterator->getNextObject());
        
        if (display) {
            IOLog("%s::Got active display\n", getName());
            
            framebuffer = OSDynamicCast(IOFramebuffer, display->getParentEntry(gIOServicePlane)->getParentEntry(gIOServicePlane));
            
            if (framebuffer)
                IOLog("%s::Got active framebuffer\n", getName());
        }
        
        iterator->release();
    }

    return framebuffer;
}

void VoodooI2CAccelerometerSensor::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    if (!x_axis || !y_axis || !z_axis)
        return;
    
    if (x_axis->getReportID() != report_id)
        return;

    if (!active_framebuffer)
        active_framebuffer = getFramebuffer();
    
    SInt16 x_axis_value = *(SInt16*)x_axis->getDataValue()->getBytesNoCopy();
    SInt16 y_axis_value = *(SInt16*)y_axis->getDataValue()->getBytesNoCopy();
    SInt16 z_axis_value = *(SInt16*)z_axis->getDataValue()->getBytesNoCopy();

    UInt16 x_axis_absolute = x_axis_value >= 0 ? x_axis_value : -x_axis_value;
    UInt16 y_axis_absolute = y_axis_value >= 0 ? y_axis_value : -y_axis_value;
    UInt16 z_axis_absolute = z_axis_value >= 0 ? z_axis_value : -z_axis_value;
    
    IOOptionBits rotation_state = kIOScaleRotate0;

    if (z_axis_absolute > 4 * x_axis_absolute && z_axis_absolute > 4 * y_axis_absolute) {
        rotation_state = kIOScaleRotateFlat;
    } else if (3 * y_axis_absolute > 2 * x_axis_absolute) {
        if (y_axis_value > 0)
            rotation_state = kIOScaleRotate180;
        else
            rotation_state = kIOScaleRotate0;
    } else {
        if (x_axis_value > 0)
            rotation_state = kIOScaleRotate90;
        else
            rotation_state = kIOScaleRotate270;
    }
    
    if (current_rotation != rotation_state)
        rotateDevice(rotation_state);
}

void VoodooI2CAccelerometerSensor::rotateDevice(IOOptionBits rotation_state) {
    if (!active_framebuffer)
        return;
    
    if (rotation_state == kIOScaleRotateFlat)
        return;

    active_framebuffer->requestProbe(kIOFBSetTransform | (rotation_state) << 16);

    current_rotation = rotation_state;
}

IOReturn VoodooI2CAccelerometerSensor::setPowerState(unsigned long whichState, IOService* whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            awake = false;
        }
    } else {
        if (!awake) {
            setElementValue(change_sensitivity, 3);
            
            awake = true;
        }
    }
    return kIOPMAckImplied;
}
bool VoodooI2CAccelerometerSensor::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    IOLog("%s Found an accelerometer sensor\n", getName());
    
    for (int i = 0; i < element->getChildElements()->getCount(); i++) {
        IOHIDElement* child = OSDynamicCast(IOHIDElement, element->getChildElements()->getObject(i));
        
        if (!child)
            continue;
        
        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Acceleration_Axis_X)) {
            x_axis = child;
            continue;
        }
        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Acceleration_Axis_Y)) {
            y_axis = child;
            continue;
        }
        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Acceleration_Axis_Z)) {
            z_axis = child;
            continue;
        }
        
        if (child->conformsTo(kHIDPage_Sensor, 0x1452)) {
            change_sensitivity = child;
            continue;
        }
    }
    
    if (!x_axis || !y_axis || !z_axis)
        return false;
    
    if (change_sensitivity) {
        // Hack to bypass a bug in Apple's HID stack which can't properly handle the structure of HID Sensor
        // reports.
        setElementValue(change_sensitivity, 3);
    }

    active_framebuffer = getFramebuffer();
    
    return true;
}

VoodooI2CSensor* VoodooI2CAccelerometerSensor::withElement(IOHIDElement* sensor_element, IOService* event_driver) {
    VoodooI2CSensor* sensor = OSTypeAlloc(VoodooI2CAccelerometerSensor);
    
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
