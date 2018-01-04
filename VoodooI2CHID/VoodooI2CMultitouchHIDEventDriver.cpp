//
//  VoodooI2CMultitouchHIDEventDriver.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CMultitouchHIDEventDriver.hpp"
#include <IOKit/hid/IOHIDInterface.h>

#define GetReportType( type )                                               \
((type <= kIOHIDElementTypeInput_ScanCodes) ? kIOHIDReportTypeInput :   \
(type <= kIOHIDElementTypeOutput) ? kIOHIDReportTypeOutput :            \
(type <= kIOHIDElementTypeFeature) ? kIOHIDReportTypeFeature : -1)

#define super IOHIDEventService
OSDefineMetaClassAndStructors(VoodooI2CMultitouchHIDEventDriver, IOHIDEventService);

static int pow(int x, int y){
    int ret = 1;
    while (y > 0){
        ret *= x;
        y--;
    }
    return ret;
}

void VoodooI2CMultitouchHIDEventDriver::calibrateJustifiedPreferredStateElement(IOHIDElement* element, SInt32 removal_percentage) {
    UInt32 sat_min   = element->getLogicalMin();
    UInt32 sat_max   = element->getLogicalMax();
    UInt32 diff     = ((sat_max - sat_min) * removal_percentage) / 200;
    sat_min          += diff;
    sat_max          -= diff;
    
    element->setCalibration(0, 1, sat_min, sat_max);
}

bool VoodooI2CMultitouchHIDEventDriver::didTerminate(IOService* provider, IOOptionBits options, bool* defer) {
    if (hid_interface)
        hid_interface->close(this);
    hid_interface = NULL;
    
    return super::didTerminate(provider, options, defer);
}

void VoodooI2CMultitouchHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    if (!readyForReports() || report_type != kIOHIDReportTypeInput)
        return;

    handleDigitizerReport(timestamp, report_id);
    
    if (!digitiser.hybrid_reporting || (digitiser.hybrid_current_transducer_id == digitiser.hybrid_current_contact_count - 1)) {
        /*
        for (int i = 0; i < digitiser.transducers->getCount(); i++) {
            VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(i));
            if (transducer->is_valid)
                IOLog("Transducer ID: %d, X: %d, Y: %d, tip_switch: %d\n", i, transducer->coordinates.x.value(), transducer->coordinates.y.value(), transducer->tip_switch.value());
        }
         */
        VoodooI2CMultitouchEvent event;
        if (!digitiser.hybrid_reporting)
            event.contact_count = digitiser.contact_count->getValue();
        else
            event.contact_count = digitiser.hybrid_current_contact_count;
        event.transducers = digitiser.transducers;

        multitouch_interface->handleInterruptReport(event, timestamp);

        digitiser.hybrid_current_contact_count = 0;
        digitiser.hybrid_current_transducer_id = 0;
    }
}

void VoodooI2CMultitouchHIDEventDriver::handleDigitizerReport(AbsoluteTime timestamp, UInt32 report_id) {
    UInt32 index, count;

    if (!digitiser.transducers)
        return;
    
    OSArray* wrappers = OSArray::withCapacity(1);
    
    wrappers->merge(digitiser.styluses);
    wrappers->merge(digitiser.fingers);
    
    if (digitiser.hybrid_reporting) {
        digitiser.hybrid_current_contact_count = digitiser.contact_count->getValue() ? digitiser.contact_count->getValue() : digitiser.hybrid_current_contact_count;
    }
    
    for (index = 0, count = wrappers->getCount(); index < count; index++) {
        VoodooI2CHIDTransducerWrapper* wrapper;

        wrapper = OSDynamicCast(VoodooI2CHIDTransducerWrapper, wrappers->getObject(index));
        if (!wrapper)
            continue;

        handleDigitizerTransducerReport(wrapper, timestamp, report_id);
    }
    
    // Now handle button report
    if (digitiser.button) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(0));
        setButtonState(&transducer->physical_button, 0, digitiser.button->getValue(), timestamp);
    }
}

void VoodooI2CMultitouchHIDEventDriver::handleDigitizerTransducerReport(VoodooI2CHIDTransducerWrapper* wrapper, AbsoluteTime timestamp, UInt32 report_id) {
    bool handled = false;
    UInt32 element_index = 0;
    UInt32 element_count = 0;
    VoodooI2CDigitiserTransducer* transducer;
    
    if (!wrapper->hid_element)
        return;
    
    OSArray* child_elements = wrapper->hid_element->getChildElements();
    
    if (!child_elements)
        return;
    
    if (!digitiser.hybrid_reporting)
        transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, wrapper->transducers->getObject(0));
    else {
        digitiser.hybrid_current_transducer_id = wrapper->contact_identifier->getValue() ? wrapper->contact_identifier->getValue() : 0;
        transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, wrapper->transducers->getObject(digitiser.hybrid_current_transducer_id % digitiser.transducer_multiplier));
    }
    
    for (element_index=0, element_count=child_elements->getCount(); element_index < element_count; element_index++) {
        IOHIDElement* element;
        AbsoluteTime element_timestamp;
        bool element_is_current;
        UInt32 usage_page;
        UInt32 usage;
        UInt32 value;
        
        VoodooI2CDigitiserStylus* stylus = (VoodooI2CDigitiserStylus*)transducer;
        
        element = OSDynamicCast(IOHIDElement, child_elements->getObject(element_index));
        if (!element)
            continue;
        
        element_timestamp = element->getTimeStamp();
        element_is_current = (element->getReportID() == report_id) && (CMP_ABSOLUTETIME(&timestamp, &element_timestamp)==0);
        
        transducer->id = report_id;
        transducer->timestamp = element_timestamp;
        
        usage_page = element->getUsagePage();
        usage = element->getUsage();
        value = element->getValue();
        
        switch (usage_page) {
            case kHIDPage_GenericDesktop:
                switch (usage) {
                    case kHIDUsage_GD_X:
                    {transducer->coordinates.x.update(element->getValue(), timestamp);
                        transducer->logical_max_x = element->getLogicalMax();
                        handled    |= element_is_current;}
                        break;
                    case kHIDUsage_GD_Y:
                    {transducer->coordinates.y.update(element->getValue(), timestamp);
                        transducer->logical_max_y = element->getLogicalMax();
                        handled    |= element_is_current;}
                        break;
                    case kHIDUsage_GD_Z:
                    {transducer->coordinates.z.update(element->getValue(), timestamp);
                        transducer->logical_max_z = element->getLogicalMax();
                        handled    |= element_is_current;}
                        break;
                }
                break;
            case kHIDPage_Button:
                setButtonState(&transducer->physical_button, (usage - 1), value, timestamp);
                handled    |= element_is_current;
                break;
            case kHIDPage_Digitizer:
                switch (usage) {
                    case kHIDUsage_Dig_TransducerIndex:
                    case kHIDUsage_Dig_ContactIdentifier:
                        transducer->secondary_id = value;
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_Touch:
                    case kHIDUsage_Dig_TipSwitch:
                        setButtonState(&transducer->tip_switch, 0, value, timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_InRange:
                        transducer->in_range = value != 0;
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_TipPressure:
                    case kHIDUsage_Dig_SecondaryTipSwitch:
                    {transducer->tip_pressure.update(element->getValue(), timestamp);
                        transducer->pressure_physical_max = element->getPhysicalMax();
                        handled    |= element_is_current;}
                        break;
                    case kHIDUsage_Dig_XTilt:
                        transducer->tilt_orientation.x_tilt.update(element->getScaledFixedValue(kIOHIDValueScaleTypePhysical), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_YTilt:
                        transducer->tilt_orientation.y_tilt.update(element->getScaledFixedValue(kIOHIDValueScaleTypePhysical), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_Azimuth:
                        transducer->azi_alti_orientation.azimuth.update(element->getValue(), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_Altitude:
                        transducer->azi_alti_orientation.altitude.update(element->getValue(), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_Twist:
                        transducer->azi_alti_orientation.twist.update(element->getScaledFixedValue(kIOHIDValueScaleTypePhysical), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_Width:
                        transducer->dimensions.width.update(element->getValue(), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_Height:
                        transducer->dimensions.height.update(element->getValue(), timestamp);
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_DataValid:
                    case kHIDUsage_Dig_TouchValid:
                    case kHIDUsage_Dig_Quality:
                        if (value)
                            transducer->is_valid = true;
                        else
                            transducer->is_valid = false;
                        handled    |= element_is_current;
                        break;
                    case kHIDUsage_Dig_BarrelPressure:
                        if (stylus) {
                            stylus->barrel_pressure.update(element->getScaledFixedValue(kIOHIDValueScaleTypeCalibrated), timestamp);
                            handled    |= element_is_current;
                        }
                        break;
                    case kHIDUsage_Dig_BarrelSwitch:
                        if (stylus) {
                            setButtonState(&stylus->barrel_switch, 1, value, timestamp);
                            handled    |= element_is_current;
                        }
                        break;
                    case kHIDUsage_Dig_BatteryStrength:
                        if (stylus) {
                            stylus->battery_strength = element->getValue();
                            handled |= element_is_current;
                        }
                        break;
                    case kHIDUsage_Dig_Eraser:
                        if (stylus) {
                            setButtonState (&stylus->eraser, 2, value, timestamp);
                            stylus->invert = value != 0;
                            handled    |= element_is_current;
                        }
                        break;
                    case kHIDUsage_Dig_Invert:
                        if (stylus) {
                            stylus->invert = value != 0;
                            handled    |= element_is_current;
                        }
                        break;
                    default:
                        break;
                }
                break;
        }
    }
    
    if (!handled)
        return;
}

bool VoodooI2CMultitouchHIDEventDriver::handleStart(IOService* provider) {
    hid_interface = OSDynamicCast(IOHIDInterface, provider);

    if (!hid_interface)
        return false;

    hid_interface->setProperty("VoodooI2CServices Supported", OSBoolean::withBoolean(true));

    if (!hid_interface->open(this, 0, OSMemberFunctionCast(IOHIDInterface::InterruptReportAction, this, &VoodooI2CMultitouchHIDEventDriver::handleInterruptReport), NULL))
        return false;
    
    hid_device = OSDynamicCast(VoodooI2CHIDDevice, hid_interface->getParentEntry(gIOServicePlane));
    
    if (!hid_device)
        return false;

    OSObject* object = copyProperty(kIOHIDAbsoluteAxisBoundsRemovalPercentage, gIOServicePlane);

    OSNumber* number = OSDynamicCast(OSNumber, object);

    if (number) {
        absolute_axis_removal_percentage = number->unsigned32BitValue();
    }

    OSSafeReleaseNULL(object);

    publishMultitouchInterface();
    
    digitiser.fingers = OSArray::withCapacity(1);
    
    if (!digitiser.fingers)
        return false;
    
    digitiser.styluses = OSArray::withCapacity(1);
    
    if (!digitiser.styluses)
        return false;

    digitiser.transducers = OSArray::withCapacity(1);

    if (!digitiser.transducers)
        return false;

    if (parseElements() != kIOReturnSuccess) {
        IOLog("%s::%s Could not parse multitouch elements\n", getName(), hid_device->name);
        return false;
    }
    
    setDigitizerProperties();

    PMinit();
    hid_interface->joinPMtree(this);
    registerPowerDriver(this, VoodooI2CIOPMPowerStates, kVoodooI2CIOPMNumberPowerStates);

    return true;
}

void VoodooI2CMultitouchHIDEventDriver::handleStop(IOService* provider) {
    if (digitiser.transducers) {
        OSSafeReleaseNULL(digitiser.transducers);
    }
    
    if (supported_elements)
        OSSafeReleaseNULL(supported_elements);
    
    if (digitiser.contact_count)
        OSSafeReleaseNULL(digitiser.contact_count);

    if (digitiser.input_mode)
        OSSafeReleaseNULL(digitiser.input_mode);
    
    if (digitiser.contact_count_maximum)
        OSSafeReleaseNULL(digitiser.contact_count_maximum);
    
    if (digitiser.button)
        OSSafeReleaseNULL(digitiser.button);

    if (multitouch_interface) {
        multitouch_interface->stop(this);
        multitouch_interface->release();
        multitouch_interface = NULL;
    }

    PMstop();
}

IOReturn VoodooI2CMultitouchHIDEventDriver::parseDigitizerElement(IOHIDElement* digitiser_element) {
    OSArray* children = OSArray::withCapacity(1);
    children = digitiser_element->getChildElements();
    
    if (!children)
        return kIOReturnNotFound;
    
    // Let's parse the configuration page first
    
    if (digitiser_element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_DeviceConfiguration)) {
        for (int i = 0; i < children->getCount(); i++) {
            IOHIDElement* element = OSDynamicCast(IOHIDElement, children->getObject(i));
            
            if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_Finger)) {
                OSArray* sub_array = element->getChildElements();

                for (int j = 0; j < sub_array->getCount(); j++) {
                    IOHIDElement* sub_element = OSDynamicCast(IOHIDElement, sub_array->getObject(j));

                    if (sub_element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_DeviceMode))
                        digitiser.input_mode = sub_element;
                }
            }
        }
        
        return kIOReturnSuccess;
    }

    for (int i = 0; i < children->getCount(); i++) {
        IOHIDElement* element = OSDynamicCast(IOHIDElement, children->getObject(i));
        
        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_Stylus)) {
            VoodooI2CHIDTransducerWrapper* wrapper = VoodooI2CHIDTransducerWrapper::withElement(element, kDigitiserTransducerStylus);
            digitiser.styluses->setObject(wrapper);
            setProperty("SupportsInk", 1, 32);
            continue;
        }
        
        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_Finger)) {
            VoodooI2CHIDTransducerWrapper* wrapper = VoodooI2CHIDTransducerWrapper::withElement(element, kDigitiserTransducerFinger);
            digitiser.fingers->setObject(wrapper);
            
            // Let's grab the logical and physical min/max while we are here
            // also the contact identifier

            OSArray* sub_array = element->getChildElements();
            
            for (int j = 0; j < sub_array->getCount(); j++) {
                IOHIDElement* sub_element = OSDynamicCast(IOHIDElement, sub_array->getObject(j));

                if (sub_element->conformsTo(kHIDPage_GenericDesktop, kHIDUsage_GD_X)) {
                    if (!multitouch_interface->logical_max_x) {
                        multitouch_interface->logical_max_x = sub_element->getLogicalMax();
                        
                        UInt32 raw_physical_max_x = sub_element->getPhysicalMax();
                        
                        UInt8 raw_unit_exponent = sub_element->getUnitExponent();
                        if (raw_unit_exponent >> 3){
                            raw_unit_exponent = raw_unit_exponent | 0xf0; //Raise the 4-bit int to an 8-bit int
                        }
                        SInt8 unit_exponent = *(SInt8 *)&raw_unit_exponent;
                        
                        UInt32 physical_max_x = raw_physical_max_x;
                        
                        physical_max_x *= pow(10,(-(unit_exponent - -2)));
                        
                        if (sub_element->getUnit() == 0x13){
                            physical_max_x *= 2.54;
                        }
                        
                        multitouch_interface->physical_max_x = physical_max_x;
                    }
                } else if (sub_element->conformsTo(kHIDPage_GenericDesktop, kHIDUsage_GD_Y)) {
                    if (!multitouch_interface->logical_max_y) {
                        multitouch_interface->logical_max_y = sub_element->getLogicalMax();
                        
                        UInt32 raw_physical_max_y = sub_element->getPhysicalMax();
                        
                        UInt8 raw_unit_exponent = sub_element->getUnitExponent();
                        if (raw_unit_exponent >> 3){
                            raw_unit_exponent = raw_unit_exponent | 0xf0; //Raise the 4-bit int to an 8-bit int
                        }
                        SInt8 unit_exponent = *(SInt8 *)&raw_unit_exponent;
                        
                        UInt32 physical_max_y = raw_physical_max_y;
                        
                        physical_max_y *= pow(10,(-(unit_exponent - -2)));
                        
                        if (sub_element->getUnit() == 0x13){
                            physical_max_y *= 2.54;
                        }
                        
                        multitouch_interface->physical_max_y = physical_max_y;
                    }
                } else if (sub_element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_ContactIdentifier)) {
                    wrapper->contact_identifier = sub_element;
                }
            }
            continue;
        }

        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_ContactCount)) {
            digitiser.contact_count = element;
            digitiser.contact_count->retain();
            continue;
        }

        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_DeviceMode)) {
            digitiser.input_mode = element;
            digitiser.input_mode->retain();
            continue;
        }
    
        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_ContactCountMaximum)) {
            digitiser.contact_count_maximum = element;
            digitiser.contact_count_maximum->retain();
            continue;
        }
        
        if (element->conformsTo(kHIDPage_Button, kHIDUsage_Button_1)) {
            digitiser.button = element;
            digitiser.button->retain();
        }
    }

    return kIOReturnSuccess;
}
    
IOReturn VoodooI2CMultitouchHIDEventDriver::parseElements() {
    int index, count;

    supported_elements = OSDynamicCast(OSArray, hid_device->getProperty(kIOHIDElementKey));

    if (!supported_elements)
        return kIOReturnNotFound;

    supported_elements->retain();

    for (index=0, count = supported_elements->getCount(); index < count; index++) {
        IOHIDElement* element = NULL;
        
        element = OSDynamicCast(IOHIDElement, supported_elements->getObject(index));
        if (!element)
            continue;

        if (element->getUsage() == 0)
            continue;

        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_Pen)
            || element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_TouchScreen)
            || element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_TouchPad)
            || element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_DeviceConfiguration)
            )
            parseDigitizerElement(element);
        }

    if (digitiser.styluses->getCount() == 0 && digitiser.fingers->getCount() == 0)
        return kIOReturnError;

    UInt8 contact_count_maximum = hid_device->getElementValue(digitiser.contact_count_maximum);

    float transducer_multiplier = (1.0f*contact_count_maximum)/(1.0f*digitiser.fingers->getCount());

    if (static_cast<float>(static_cast<int>(transducer_multiplier)) != transducer_multiplier) {
        IOLog("%s::%s Unknown digitiser type: got %d finger collections and a %d maximum contact count, aborting\n", getName(), hid_device->name, digitiser.fingers->getCount(), contact_count_maximum);
        return kIOReturnInvalid;
    }
    
    digitiser.transducer_multiplier = static_cast<int>(transducer_multiplier);
    
    if (digitiser.transducer_multiplier != 1)
        digitiser.hybrid_reporting = true;
    
    for (int i = 0; i < digitiser.styluses->getCount(); i++) {
        VoodooI2CHIDTransducerWrapper* wrapper = OSDynamicCast(VoodooI2CHIDTransducerWrapper, digitiser.fingers->getObject(i));
        
        for (int j = 0; j < digitiser.transducer_multiplier; j++) {
            VoodooI2CDigitiserTransducer* transducer = VoodooI2CDigitiserStylus::stylus(wrapper->type, wrapper->hid_element);
            wrapper->transducers->setObject(transducer);
            
            digitiser.transducers->setObject(transducer);
        }
    }
    
    for (int i=0; i < digitiser.fingers->getCount(); i++) {
        VoodooI2CHIDTransducerWrapper* wrapper = OSDynamicCast(VoodooI2CHIDTransducerWrapper, digitiser.fingers->getObject(i));

        for (int j = 0; j < digitiser.transducer_multiplier; j++) {
            VoodooI2CDigitiserTransducer* transducer = VoodooI2CDigitiserTransducer::transducer(wrapper->type, wrapper->hid_element);
            wrapper->transducers->setObject(transducer);

            digitiser.transducers->setObject(transducer);
        }
    }

    return kIOReturnSuccess;
}

IOReturn VoodooI2CMultitouchHIDEventDriver::publishMultitouchInterface() {
    OSBoolean* integrated;

    multitouch_interface = new VoodooI2CMultitouchInterface;
    
    if (!multitouch_interface || !multitouch_interface->init(NULL)) {
        goto exit;
    }
    
    if (!multitouch_interface->attach(this)) {
        goto exit;
    }
    
    if (!multitouch_interface->start(this)) {
        goto exit;
    }

    integrated = OSDynamicCast(OSBoolean, getProperty(kIOHIDDisplayIntegratedKey));

    if (integrated)
        multitouch_interface->setProperty(kIOHIDDisplayIntegratedKey, integrated);
    else
        multitouch_interface->setProperty(kIOHIDDisplayIntegratedKey, OSBoolean::withBoolean(false));

    multitouch_interface->setProperty(kIOHIDVendorIDKey, getVendorID(), 32);
    multitouch_interface->setProperty(kIOHIDProductIDKey, getProductID(), 32);

    multitouch_interface->registerService();
    
    return kIOReturnSuccess;
    
exit:
    if (multitouch_interface) {
        multitouch_interface->stop(this);
        multitouch_interface->release();
        multitouch_interface = NULL;
    }
    
    return kIOReturnError;
}

inline void VoodooI2CMultitouchHIDEventDriver::setButtonState(DigitiserTransducerButtonState* state, UInt32 bit, UInt32 value, AbsoluteTime timestamp) {
    UInt32 buttonMask = (1 << bit);
    
    if ( value != 0 )
        state->update(state->current.value |= buttonMask, timestamp);
    else
        state->update(state->current.value &= ~buttonMask, timestamp);
}

void VoodooI2CMultitouchHIDEventDriver::setDigitizerProperties() {
    OSDictionary* properties = OSDictionary::withCapacity(4);

    if (!properties)
        return;

    if (!digitiser.transducers)
        return;

    properties->setObject("Contact Count Element", digitiser.contact_count);
    properties->setObject("Input Mode Element", digitiser.input_mode);
    properties->setObject("Contact Count Maximum  Element", digitiser.contact_count_maximum);
    properties->setObject("Button Element", digitiser.button);
    properties->setObject("Transducers", digitiser.transducers);

    setProperty("Digitizer", properties);
    
exit:
    OSSafeReleaseNULL(properties);
}

IOReturn VoodooI2CMultitouchHIDEventDriver::setPowerState(unsigned long whichState, IOService* whatDevice) {
    return kIOPMAckImplied;
}

bool VoodooI2CMultitouchHIDEventDriver::start(IOService* provider) {
    if (!super::start(provider))
        return false;

    setProperty("VoodooI2CServices Supported", OSBoolean::withBoolean(true));

    return true;
}
