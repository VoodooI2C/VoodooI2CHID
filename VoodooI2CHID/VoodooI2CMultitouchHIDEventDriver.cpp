//
//  VoodooI2CMultitouchHIDEventDriver.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CMultitouchHIDEventDriver.hpp"
#include <IOKit/hid/IOHIDInterface.h>
#include "AppleHIDUsageTables.h"

#define SET_NUMBER(key, num) do { \
tmpNumber = OSNumber::withNumber(num, 32); \
if (tmpNumber) { \
kbEnableEventProps->setObject(key, tmpNumber); \
tmpNumber->release(); \
} \
}while (0);

#define GetReportType( type )                                               \
((type <= kIOHIDElementTypeInput_ScanCodes) ? kIOHIDReportTypeInput :   \
(type <= kIOHIDElementTypeOutput) ? kIOHIDReportTypeOutput :            \
(type <= kIOHIDElementTypeFeature) ? kIOHIDReportTypeFeature : -1)

#define super IOHIDEventService
OSDefineMetaClassAndStructors(VoodooI2CMultitouchHIDEventDriver, IOHIDEventService);

static int pow(int x, int y) {
    int ret = 1;
    while (y > 0){
        ret *= x;
        y--;
    }
    return ret;
}

static int roundUp(int numToRound, int multiple) {
    if (multiple == 0)
        return numToRound;
    
    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;
    
    return numToRound + multiple - remainder;
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

UInt32 VoodooI2CMultitouchHIDEventDriver::getElementValue(IOHIDElement* element) {
    IOHIDElementCookie cookie = element->getCookie();
    
    if (!cookie)
        return 0;
    
    hid_device->updateElementValues(&cookie);
    
    return element->getValue();
}

const char* VoodooI2CMultitouchHIDEventDriver::getProductName() {
    VoodooI2CHIDDevice* i2c_hid_device = OSDynamicCast(VoodooI2CHIDDevice, hid_device);
    
    if (i2c_hid_device)
        return i2c_hid_device->name;
    
    OSString* name = getProduct();
    
    return name->getCStringNoCopy();
}

void VoodooI2CMultitouchHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    if (!readyForReports() || report_type != kIOHIDReportTypeInput)
        return;
    
    if (digitiser.contact_count->getValue()) {
        digitiser.current_contact_count = digitiser.contact_count->getValue();
        
        UInt8 finger_count = digitiser.fingers->getCount();
        digitiser.report_count = static_cast<int>(roundUp(digitiser.current_contact_count, finger_count)/finger_count);
        digitiser.current_report = 1;
    }
    
    handleDigitizerReport(timestamp, report_id);
    
    if (digitiser.current_report == digitiser.report_count) {
        
        VoodooI2CMultitouchEvent event;
        event.contact_count = digitiser.current_contact_count;
        event.transducers = digitiser.transducers;
        
        multitouch_interface->handleInterruptReport(event, timestamp);
        
        digitiser.report_count = 1;
        digitiser.current_report = 1;
    } else {
        digitiser.current_report++;
    }
    
    handleKeboardReport(timestamp, report_id);
}

void VoodooI2CMultitouchHIDEventDriver::handleDigitizerReport(AbsoluteTime timestamp, UInt32 report_id) {
    if (!digitiser.transducers)
        return;
    
    VoodooI2CHIDTransducerWrapper* wrapper;
    
    wrapper = OSDynamicCast(VoodooI2CHIDTransducerWrapper, digitiser.wrappers->getObject(digitiser.current_report - 1));
    
    if (!wrapper)
        return;
    
    // Check if we are sending the report to the right wrapper, 99% of the time, `digitiser.current_report - 1` will
    // be the correct index but in rare circumstances, it won't be so we should ensure we have the right index
    
    UInt8 finger_count = digitiser.fingers->getCount();
    
    if (!wrapper->first_identifier)
        return;
    
    UInt8 first_identifier = wrapper->first_identifier->getValue() ? wrapper->first_identifier->getValue() : 0;
    
    UInt8 actual_index = static_cast<int>(roundUp(first_identifier + 1, finger_count)/finger_count) - 1;
    
    if (actual_index != digitiser.current_report - 1) {
        wrapper = OSDynamicCast(VoodooI2CHIDTransducerWrapper, digitiser.wrappers->getObject(actual_index));
        if (!wrapper)
            return;
    }
    
    for (int i = 0; i < wrapper->transducers->getCount(); i++) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, wrapper->transducers->getObject(i));
        handleDigitizerTransducerReport(transducer, timestamp, report_id);
    }
    
    // Now handle button report
    if (digitiser.button) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(0));
        setButtonState(&transducer->physical_button, 0, digitiser.button->getValue(), timestamp);
    }
    
    if (digitiser.styluses->getCount() > 0) {
        // The stylus wrapper is the last one
        wrapper = OSDynamicCast(VoodooI2CHIDTransducerWrapper, digitiser.wrappers->getLastObject());
        
        VoodooI2CDigitiserStylus* stylus = OSDynamicCast(VoodooI2CDigitiserStylus, wrapper->transducers->getObject(0));
        
        IOHIDElement* element = OSDynamicCast(IOHIDElement, stylus->collection->getChildElements()->getObject(0));
        
        if (element && report_id == element->getReportID()) {
            handleDigitizerTransducerReport(stylus, timestamp, report_id);
        }
    }
}

void VoodooI2CMultitouchHIDEventDriver::handleDigitizerTransducerReport(VoodooI2CDigitiserTransducer* transducer, AbsoluteTime timestamp, UInt32 report_id) {
    bool handled = false;
    bool has_confidence = false;
    UInt32 element_index = 0;
    UInt32 element_count = 0;
    
    if (!transducer->collection)
        return;
    
    OSArray* child_elements = transducer->collection->getChildElements();
    
    if (!child_elements)
        return;
    
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
                    {
                        transducer->coordinates.x.update(element->getValue(), timestamp);
                        transducer->logical_max_x = element->getLogicalMax();
                        handled    |= element_is_current;
                    }
                        break;
                    case kHIDUsage_GD_Y:
                    {
                        transducer->coordinates.y.update(element->getValue(), timestamp);
                        transducer->logical_max_y = element->getLogicalMax();
                        handled    |= element_is_current;
                    }
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
                        has_confidence = true;
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
    
    if (!has_confidence)
        transducer->is_valid = true;
    
    if (!handled)
        return;
}

void VoodooI2CMultitouchHIDEventDriver::handleKeboardReport(AbsoluteTime timeStamp, UInt32 reportID)
{
    UInt32      volumeHandled   = 0;
    UInt32      volumeState     = 0;
    UInt32      index, count;
    
    if(!keyboard.elements)
        goto exit;
    
    for (index=0, count=keyboard.elements->getCount(); index<count; index++) {
        IOHIDElement *  element;
        AbsoluteTime    elementTimeStamp;
        UInt32          usagePage, usage, value, preValue;
        
        element = OSDynamicCast(IOHIDElement, keyboard.elements->getObject(index));
        if ( !element )
            continue;
        
        if ( element->getReportID() != reportID )
            continue;
        
        elementTimeStamp = element->getTimeStamp();
        if ( CMP_ABSOLUTETIME(&timeStamp, &elementTimeStamp) != 0 )
            continue;
        
        preValue    = element->getValue(kIOHIDValueOptionsFlagPrevious) != 0;
        value       = element->getValue() != 0;
        
        if ( value == preValue )
            continue;
        
        usagePage   = element->getUsagePage();
        usage       = element->getUsage();
        
        if ( usagePage == kHIDPage_Consumer ) {
            bool suppress = true;
            switch ( usage ) {
                case kHIDUsage_Csmr_VolumeIncrement:
                    volumeHandled   |= 0x1;
                    volumeState     |= (value) ? 0x1:0;
                    break;
                case kHIDUsage_Csmr_VolumeDecrement:
                    volumeHandled   |= 0x2;
                    volumeState     |= (value) ? 0x2:0;
                    break;
                case kHIDUsage_Csmr_Mute:
                    volumeHandled   |= 0x4;
                    volumeState     |= (value) ? 0x4:0;
                    break;
                default:
                    suppress = false;
                    break;
            }
            
            if ( suppress )
                continue;
        }
        
        /* Disabled due to non-avaiablitity of identifiers
         
         else if (usage == kHIDUsage_KeyboardPower && usagePage == kHIDPage_KeyboardOrKeypad) {
         if (value == 0) {
         setProperty(kIOHIDKeyboardEnabledKey, kOSBooleanFalse);
         }
         
         else {
         setProperty(kIOHIDKeyboardEnabledKey, kOSBooleanTrue);
         }
         }*/
        
        dispatchKeyboardEvent(timeStamp, usagePage, usage, value);
    }
    
    // RY: Handle the case where Vol Increment, Decrement, and Mute are all down
    // If such an event occurs, it is likely that the device is defective,
    // and should be ignored.
    if ( (volumeState != 0x7) && (volumeHandled != 0x7) ) {
        // Volume Increment
        if ( volumeHandled & 0x1 )
            dispatchKeyboardEvent(timeStamp, kHIDPage_Consumer, kHIDUsage_Csmr_VolumeIncrement, ((volumeState & 0x1) != 0));
        // Volume Decrement
        if ( volumeHandled & 0x2 )
            dispatchKeyboardEvent(timeStamp, kHIDPage_Consumer, kHIDUsage_Csmr_VolumeDecrement, ((volumeState & 0x2) != 0));
        // Volume Mute
        if ( volumeHandled & 0x4 )
            dispatchKeyboardEvent(timeStamp, kHIDPage_Consumer, kHIDUsage_Csmr_Mute, ((volumeState & 0x4) != 0));
    }
    
exit:
    return;
}

bool VoodooI2CMultitouchHIDEventDriver::handleStart(IOService* provider) {
    hid_interface = OSDynamicCast(IOHIDInterface, provider);
    
    if (!hid_interface)
        return false;
    
    hid_interface->setProperty("VoodooI2CServices Supported", OSBoolean::withBoolean(true));
    
    if (!hid_interface->open(this, 0, OSMemberFunctionCast(IOHIDInterface::InterruptReportAction, this, &VoodooI2CMultitouchHIDEventDriver::handleInterruptReport), NULL))
        return false;
    
    hid_device = OSDynamicCast(IOHIDDevice, hid_interface->getParentEntry(gIOServicePlane));
    
    if (!hid_device)
        return false;
    
    name = getProductName();
    
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
        IOLog("%s::%s Could not parse multitouch elements\n", getName(), name);
        return false;
    }
    
    setDigitizerProperties();
    setKeyboardProperties();
    
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
            digitiser.styluses->setObject(element);
            setProperty("SupportsInk", 1, 32);
            continue;
        }
        
        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_Finger)) {
            digitiser.fingers->setObject(element);
            
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
                        
                        physical_max_x *= pow(10,(unit_exponent - -2));
                        
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
                        
                        physical_max_y *= pow(10,(unit_exponent - -2));
                        
                        if (sub_element->getUnit() == 0x13){
                            physical_max_y *= 2.54;
                        }
                        
                        multitouch_interface->physical_max_y = physical_max_y;
                    }
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

bool VoodooI2CMultitouchHIDEventDriver::parseKeyboardElement(IOHIDElement * element)
{
    UInt32 usagePage    = element->getUsagePage();
    UInt32 usage        = element->getUsage();
    bool   store        = false;
    
    if ( !keyboard.elements ) {
        keyboard.elements = OSArray::withCapacity(4);
        if(!keyboard.elements)
            goto exit;
    }
    
    switch ( usagePage ) {
        case kHIDPage_GenericDesktop:
            switch ( usage ) {
                case kHIDUsage_GD_Start:
                case kHIDUsage_GD_Select:
                case kHIDUsage_GD_SystemPowerDown:
                case kHIDUsage_GD_SystemSleep:
                case kHIDUsage_GD_SystemWakeUp:
                case kHIDUsage_GD_SystemContextMenu:
                case kHIDUsage_GD_SystemMainMenu:
                case kHIDUsage_GD_SystemAppMenu:
                case kHIDUsage_GD_SystemMenuHelp:
                case kHIDUsage_GD_SystemMenuExit:
                case kHIDUsage_GD_SystemMenuSelect:
                case kHIDUsage_GD_SystemMenuRight:
                case kHIDUsage_GD_SystemMenuLeft:
                case kHIDUsage_GD_SystemMenuUp:
                case kHIDUsage_GD_SystemMenuDown:
                case kHIDUsage_GD_DPadUp:
                case kHIDUsage_GD_DPadDown:
                case kHIDUsage_GD_DPadRight:
                case kHIDUsage_GD_DPadLeft:
                    store = true;
                    break;
            }
            break;
        case kHIDPage_KeyboardOrKeypad:
            if (( usage < kHIDUsage_KeyboardA ) || ( usage > kHIDUsage_KeyboardRightGUI ))
                break;
            
            // This usage is used to let the OS know if a keyboard is in an enabled state where
            // user input is possible
            
            if (usage == kHIDUsage_KeyboardPower) {
                OSDictionary * kbEnableEventProps   = NULL;
                OSNumber * tmpNumber                = NULL;
                UInt32 value                        = NULL;
                
                // To avoid problems with un-intentional clearing of the flag
                // we require this report to be a feature report so that the current
                // state can be polled if necessary
                
                if (element->getType() == kIOHIDElementTypeFeature) {
                    value = element->getValue(kIOHIDValueOptionsUpdateElementValues);
                    
                    kbEnableEventProps = OSDictionary::withCapacity(3);
                    if (!kbEnableEventProps)
                        break;
                    
                    /* Since the identifiers are not defined, better comment out
                     
                     SET_NUMBER(kIOHIDKeyboardEnabledEventEventTypeKey, kIOHIDEventTypeKeyboard);
                     SET_NUMBER(kIOHIDKeyboardEnabledEventUsagePageKey, kHIDPage_KeyboardOrKeypad);
                     SET_NUMBER(kIOHIDKeyboardEnabledEventUsageKey, kHIDUsage_KeyboardPower);
                     
                     setProperty(kIOHIDKeyboardEnabledEventKey, kbEnableEventProps);
                     setProperty(kIOHIDKeyboardEnabledByEventKey, kOSBooleanTrue);
                     setProperty(kIOHIDKeyboardEnabledKey, value ? kOSBooleanTrue : kOSBooleanFalse);*/
                    
                    kbEnableEventProps->release();
                }
                
                store = true;
                break;
            }
        case kHIDPage_Consumer:
            if (usage == kHIDUsage_Csmr_ACKeyboardLayoutSelect)
                setProperty(kIOHIDSupportsGlobeKeyKey, kOSBooleanTrue);
        case kHIDPage_Telephony:
            store = true;
            break;
        case kHIDPage_AppleVendorTopCase:
            if (keyboard.appleVendorSupported) {
                switch (usage) {
                    case kHIDUsage_AV_TopCase_BrightnessDown:
                    case kHIDUsage_AV_TopCase_BrightnessUp:
                    case kHIDUsage_AV_TopCase_IlluminationDown:
                    case kHIDUsage_AV_TopCase_IlluminationUp:
                    case kHIDUsage_AV_TopCase_KeyboardFn:
                        store = true;
                        break;
                }
            }
            break;
        case kHIDPage_AppleVendorKeyboard:
            if (keyboard.appleVendorSupported) {
                switch (usage) {
                    case kHIDUsage_AppleVendorKeyboard_Spotlight:
                    case kHIDUsage_AppleVendorKeyboard_Dashboard:
                    case kHIDUsage_AppleVendorKeyboard_Function:
                    case kHIDUsage_AppleVendorKeyboard_Launchpad:
                    case kHIDUsage_AppleVendorKeyboard_Reserved:
                    case kHIDUsage_AppleVendorKeyboard_CapsLockDelayEnable:
                    case kHIDUsage_AppleVendorKeyboard_PowerState:
                    case kHIDUsage_AppleVendorKeyboard_Expose_All:
                    case kHIDUsage_AppleVendorKeyboard_Expose_Desktop:
                    case kHIDUsage_AppleVendorKeyboard_Brightness_Up:
                    case kHIDUsage_AppleVendorKeyboard_Brightness_Down:
                    case kHIDUsage_AppleVendorKeyboard_Language:
                        store = true;
                        break;
                }
            }
            break;
    }
    
    if(!store)
        goto exit;
    
    keyboard.elements->setObject(element);
    
exit:
    return store;
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
        
        if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_TouchScreen))
            multitouch_interface->setProperty(kIOHIDDisplayIntegratedKey, OSBoolean::withBoolean(true));
    }
    
    
    if (digitiser.styluses->getCount() == 0 && digitiser.fingers->getCount() == 0)
        return kIOReturnError;
    
    UInt8 contact_count_maximum = getElementValue(digitiser.contact_count_maximum);
    
    float wrapper_count = (1.0f*contact_count_maximum)/(1.0f*digitiser.fingers->getCount());
    
    if (static_cast<float>(static_cast<int>(wrapper_count)) != wrapper_count) {
        IOLog("%s::%s Unknown digitiser type: got %d finger collections and a %d maximum contact count, aborting\n", getName(), name, digitiser.fingers->getCount(), contact_count_maximum);
        return kIOReturnInvalid;
    }
    
    digitiser.wrappers = OSArray::withCapacity(wrapper_count);
    
    for (int i = 0; i < static_cast<int>(wrapper_count); i++) {
        VoodooI2CHIDTransducerWrapper* wrapper = VoodooI2CHIDTransducerWrapper::wrapper();
        digitiser.wrappers->setObject(wrapper);
        
        for (int j = 0; j < digitiser.fingers->getCount(); j++) {
            IOHIDElement* finger = OSDynamicCast(IOHIDElement, digitiser.fingers->getObject(j));
            
            VoodooI2CDigitiserTransducer* transducer = VoodooI2CDigitiserTransducer::transducer(kDigitiserTransducerFinger, finger);
            
            wrapper->transducers->setObject(transducer);
            digitiser.transducers->setObject(transducer);
        }
        
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, wrapper->transducers->getObject(0));
        
        for (int j = 0; j < transducer->collection->getChildElements()->getCount(); j++) {
            IOHIDElement* element = OSDynamicCast(IOHIDElement, transducer->collection->getChildElements()->getObject(j));
            
            if (!element)
                continue;
            
            if (element->conformsTo(kHIDPage_Digitizer, kHIDUsage_Dig_ContactIdentifier))
                wrapper->first_identifier = element;
        }
        
    }
    
    // Add stylus as the final wrapper
    
    if (digitiser.styluses->getCount()) {
        VoodooI2CHIDTransducerWrapper* stylus_wrapper = VoodooI2CHIDTransducerWrapper::wrapper();
        digitiser.wrappers->setObject(stylus_wrapper);
        
        IOHIDElement* stylus = OSDynamicCast(IOHIDElement, digitiser.styluses->getObject(0));
        VoodooI2CDigitiserStylus* transducer = VoodooI2CDigitiserStylus::stylus(kDigitiserTransducerStylus, stylus);
        
        stylus_wrapper->transducers->setObject(transducer);
        digitiser.transducers->setObject(0, transducer);
    }
    
    //Keyboard : Loop through all createMatchingElements() and parse KeyboardElements
    
    OSArray *elementArray = hid_interface->createMatchingElements();
    keyboard.appleVendorSupported = getProperty(kIOHIDAppleVendorSupported, gIOServicePlane);
    if ( elementArray ) {
        for ( index=0, count=elementArray->getCount(); index<count; index++ ) {
            IOHIDElement *  element     = NULL;
            
            element = OSDynamicCast(IOHIDElement, elementArray->getObject(index));
            if ( !element )
                continue;
            
            if ( element->getType() == kIOHIDElementTypeCollection )
                continue;
            
            if ( element->getUsage() == 0 )
                continue;
            
            if ( parseKeyboardElement(element) )
                continue;
        }
    }
    OSSafeReleaseNULL(elementArray);
    
    return kIOReturnSuccess;
}

IOReturn VoodooI2CMultitouchHIDEventDriver::publishMultitouchInterface() {
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
    
    multitouch_interface->setProperty(kIOHIDVendorIDKey, getVendorID(), 32);
    multitouch_interface->setProperty(kIOHIDProductIDKey, getProductID(), 32);
    
    multitouch_interface->setProperty(kIOHIDDisplayIntegratedKey, OSBoolean::withBoolean(false));
    
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

void VoodooI2CMultitouchHIDEventDriver::setKeyboardProperties()
{
    OSDictionary * properties = OSDictionary::withCapacity(4);
    
    if (!properties)
        return;
    
    if (!keyboard.elements)
        return;
    
    properties->setObject(kIOHIDElementKey, keyboard.elements);
    
    setProperty("Keyboard", properties);
    
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
