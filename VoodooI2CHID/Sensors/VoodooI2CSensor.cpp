//
//  VoodooI2CSensor.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 26/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CSensorHubEventDriver.hpp"
#include "VoodooI2CSensor.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooI2CSensor, IOService);

IOReturn VoodooI2CSensor::changeState(IOHIDElement* state_element, UInt16 state_usage) {
    UInt8 index = findPropertyIndex(state_element, state_usage);
    
    if (!index)
        return kIOReturnError;

    IOHIDElement* actual_element = OSDynamicCast(IOHIDElement, state_element->getChildElements()->getObject(0));

    actual_element->setValue(index);
    
    return kIOReturnSuccess;
}

UInt8 VoodooI2CSensor::findPropertyIndex(IOHIDElement* element, UInt16 usage) {
    OSArray* children = element->getChildElements();
    IOHIDElement* child;

    for (UInt8 i = 0; i < children->getCount(); i++) {
        child = OSDynamicCast(IOHIDElement, children->getObject(i));

        if (!child)
            continue;
        
        if (child->conformsTo(kHIDPage_Sensor, usage))
            return i - 1;
    }

    return 0;
}

UInt32 VoodooI2CSensor::getElementValue(IOHIDElement* element) {
    IOHIDElementCookie cookie = element->getCookie();
    
    if (!cookie)
        return 0;
    
    event_driver->hid_device->updateElementValues(&cookie);
    
    return element->getValue();
}

void VoodooI2CSensor::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    return;
}

void VoodooI2CSensor::setElementValue(IOHIDElement* updated_element, UInt32 value) {
    if (!element)
        return;

    OSData* buffer = OSData::withCapacity(1);
    bool manufacturer_already_done = false;
    bool model_already_done = false;
    
    buffer->appendByte(updated_element->getReportID(), 1);
    
    OSArray* children = element->getChildElements();

    for (int i = 0; i < children->getCount(); i++) {
        IOHIDElement* child = OSDynamicCast(IOHIDElement, children->getObject(i));

        if (!child)
            continue;
        
        IOHIDElement* element_to_use = child;
        
        if (child->getChildElements() && child->getChildElements()->getCount()) {
            IOHIDElement* sub_child = OSDynamicCast(IOHIDElement, child->getChildElements()->getObject(0));
            element_to_use = sub_child;
        }

        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Property_Manufacturer)) {
            if (manufacturer_already_done)
                continue;
            else
                manufacturer_already_done = true;
        }

        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Property_Model)) {
            if (model_already_done)
                continue;
            else
                model_already_done = true;
        }
        
        if (element_to_use->getType() != kIOHIDElementTypeFeature)
            continue;
        
        UInt32 current_value = 0;
        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Property_ReportingState))
            current_value = findPropertyIndex(child, current_reporting_state);

        if (child->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Property_PowerState))
            current_value = findPropertyIndex(child, current_power_state);

        UInt32 value_to_write = (updated_element == element_to_use ? value : current_value);
        
        buffer->appendBytes(&value_to_write, (element_to_use->getReportSize()/8) * element_to_use->getReportCount());
    }

    IOBufferMemoryDescriptor* report = IOBufferMemoryDescriptor::withBytes(buffer->getBytesNoCopy(), buffer->getLength(), kIODirectionInOut);

    buffer->release();

    event_driver->setReport(report, kIOHIDReportTypeFeature, updated_element->getReportID());

    report->release();
}

bool VoodooI2CSensor::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    event_driver = OSDynamicCast(VoodooI2CSensorHubEventDriver, provider);
    
    if (!event_driver)
        return false;
    
    OSArray* children = element->getChildElements();
    
    if (!children)
        return false;
    
    for (int i = 0; i < children->getCount(); i++) {
        IOHIDElement* child_element = OSDynamicCast(IOHIDElement, children->getObject(i));
        
        if (!child_element)
            continue;
        
        if (child_element->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Property_PowerState)) {
            power_state = child_element;
            continue;
        }
        if (child_element->conformsTo(kHIDPage_Sensor, kHIDUsage_Snsr_Property_ReportingState)) {
            reporting_state = child_element;
            continue;
        }
    }
    
    if (!power_state && !reporting_state)
        return false;

    if (changeState(power_state, kHIDUsage_Snsr_Property_PowerState_D0_FullPower) != kIOReturnSuccess) {
        IOLog("%s Could not change power state to D0\n", getName());
        return false;
    }
    
    current_power_state = kHIDUsage_Snsr_Property_PowerState_D0_FullPower;

    if (changeState(reporting_state, kHIDUsage_Snsr_Property_ReportingState_ThresholdEvents) != kIOReturnSuccess) {
        IOLog("%s Could not change reporting state to all reports\n", getName());
        return false;
    }
    
    current_reporting_state = kHIDUsage_Snsr_Property_ReportingState_ThresholdEvents;

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, VoodooI2CIOPMPowerStates, kVoodooI2CIOPMNumberPowerStates);

    return true;
}

void VoodooI2CSensor::stop(IOService* provider) {
    PMstop();
}

IOReturn VoodooI2CSensor::setPowerState(unsigned long whichState, IOService* whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            awake = false;
            
            if (changeState(power_state, kHIDUsage_Snsr_Property_PowerState_D4_PowerOff) != kIOReturnSuccess) {
                IOLog("%s Could not change power state to D0\n", getName());
                return false;
            }
            
            current_power_state = kHIDUsage_Snsr_Property_PowerState_D4_PowerOff;
            
            if (changeState(reporting_state, kHIDUsage_Snsr_Property_ReportingState_NoEvents) != kIOReturnSuccess) {
                IOLog("%s Could not change reporting state to all reports\n", getName());
                return false;
            }
            
            current_reporting_state = kHIDUsage_Snsr_Property_ReportingState_NoEvents;
        }
    } else {
        if (!awake) {
            if (changeState(power_state, kHIDUsage_Snsr_Property_PowerState_D0_FullPower) != kIOReturnSuccess) {
                IOLog("%s Could not change power state to D0\n", getName());
                return false;
            }
            
            current_power_state = kHIDUsage_Snsr_Property_PowerState_D0_FullPower;
            
            if (changeState(reporting_state, kHIDUsage_Snsr_Property_ReportingState_ThresholdEvents) != kIOReturnSuccess) {
                IOLog("%s Could not change reporting state to all reports\n", getName());
                return false;
            }
            
            current_reporting_state = kHIDUsage_Snsr_Property_ReportingState_ThresholdEvents;
            
            awake = true;
        }
    }
    return kIOPMAckImplied;
}

VoodooI2CSensor* VoodooI2CSensor::withElement(IOHIDElement* sensor_element, IOService* event_driver) {
    VoodooI2CSensor* sensor = OSTypeAlloc(VoodooI2CSensor);
    
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
