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

    VoodooI2CMultitouchEvent event;
    event.contact_count = contact_count_element->getValue();
    event.transducers = digitiser.transducers;
    
    multitouch_interface->handleInterruptReport(event, timestamp);
}

void VoodooI2CMultitouchHIDEventDriver::handleDigitizerReport(AbsoluteTime timestamp, UInt32 report_id) {
    UInt32 index, count;

    if (!digitiser.transducers)
        return;
    
    for (index = 0, count = digitiser.transducers->getCount(); index < count; index++) {
        VoodooI2CDigitiserTransducer* transducer;

        transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(index));
        if (!transducer)
            continue;

        handleDigitizerTransducerReport(transducer, timestamp, report_id);
    }
}

void VoodooI2CMultitouchHIDEventDriver::handleDigitizerTransducerReport(VoodooI2CDigitiserTransducer* transducer, AbsoluteTime timestamp, UInt32 report_id) {
    bool handled = false;
    UInt32 element_index = 0;
    UInt32 element_count = 0;
    
    if (!transducer->elements)
        return;
    
    for (element_index=0, element_count=transducer->elements->getCount(); element_index < element_count; element_index++) {
        IOHIDElement* element;
        AbsoluteTime element_timestamp;
        bool element_is_current;
        UInt32 usage_page;
        UInt32 usage;
        UInt32 value;
        
        VoodooI2CDigitiserStylus* stylus = (VoodooI2CDigitiserStylus*)transducer;
        
        element = OSDynamicCast(IOHIDElement, transducer->elements->getObject(element_index));
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

    if (parseElements() != kIOReturnSuccess)
        return false;
    
    IOLog("report: %d\n", hid_device->getElementValue(contact_count_max_element));
    
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
    
    if (contact_count_element)
        OSSafeReleaseNULL(contact_count_element);

    if (input_mode_element)
        OSSafeReleaseNULL(input_mode_element);
    
    if (contact_count_max_element)
        OSSafeReleaseNULL(contact_count_max_element);

    if (multitouch_interface) {
        multitouch_interface->stop(this);
        multitouch_interface->release();
        multitouch_interface = NULL;
    }

    PMstop();
}

IOReturn VoodooI2CMultitouchHIDEventDriver::parseDigitizerElement(IOHIDElement* element) {
    IOHIDElement* parent;
    parent = element;

    while ((parent = parent->getParentElement())) {
        bool application = false;
        switch (parent->getCollectionType()) {
            case kIOHIDElementCollectionTypeLogical:
            case kIOHIDElementCollectionTypePhysical:
                break;
            case kIOHIDElementCollectionTypeApplication:
                application = true;
                break;
            default:
                continue;
        }

        if (parent->getUsagePage() != kHIDPage_Digitizer)
            continue;

        if (application) {
            if (parent->getUsage() < kHIDUsage_Dig_Digitizer)
                continue;

            if (parent->getUsage() > kHIDUsage_Dig_DeviceConfiguration)
                continue;
        } else {
            if ( parent->getUsage() < kHIDUsage_Dig_Stylus )
                continue;

            if ( parent->getUsage() > kHIDUsage_Dig_GestureCharacter )
                continue;
        }

        break;
    }

    if (!parent)
        return kIOReturnNotFound;

    if (element->getUsage() == kHIDUsage_Dig_ContactCount) {
        contact_count_element = element;
        contact_count_element->retain();
        return kIOReturnSuccess;
    }

    if (element->getUsage() == kHIDUsage_Dig_DeviceMode) {
        input_mode_element = element;
        input_mode_element->retain();
        return kIOReturnSuccess;
    }
    
    if (element->getUsage() == kHIDUsage_Dig_ContactCountMaximum) {
        contact_count_max_element = element;
        contact_count_max_element-> retain();
        return kIOReturnSuccess;
    }

    return parseDigitizerTransducerElement(element, parent);
}

IOReturn VoodooI2CMultitouchHIDEventDriver::parseDigitizerTransducerElement(IOHIDElement* element, IOHIDElement* parent = NULL) {
    VoodooI2CDigitiserTransducer* transducer = NULL;
    bool should_calibrate = false;
    int index, count;
    
    switch (element->getUsagePage()) {
        case kHIDPage_GenericDesktop:
            switch (element->getUsage()) {
                case kHIDUsage_GD_X:
                    if (element->getFlags() & kIOHIDElementFlagsRelativeMask) {
                        IOLog("%s::%s Found relative transducer, skipping\n", getName(), hid_interface->getName());
                        return kIOReturnDeviceError;
                    }
                    should_calibrate = true;
                    if (!multitouch_interface->logical_max_x) {
                        multitouch_interface->logical_max_x = element->getLogicalMax();
                        multitouch_interface->physical_max_x = element->getPhysicalMax();
                    }
                case kHIDUsage_GD_Y:
                    if (element->getFlags() & kIOHIDElementFlagsRelativeMask) {
                        IOLog("%s::%s Found relative transducer, skipping\n", getName(), hid_interface->getName());
                        return kIOReturnDeviceError;
                    }
                    should_calibrate = true;
                    if (!multitouch_interface->logical_max_y) {
                        multitouch_interface->logical_max_y = element->getLogicalMax();
                        multitouch_interface->physical_max_y = element->getPhysicalMax();
                    }
                case kHIDUsage_GD_Z:
                    if (element->getFlags() & kIOHIDElementFlagsRelativeMask) {
                        IOLog("%s::%s Found relative transducer, skipping\n", getName(), hid_interface->getName());
                        return kIOReturnDeviceError;
                    }
                    should_calibrate = true;
                    break;
            }
            break;
    }
    
    if (GetReportType(element->getType()) != kIOHIDReportTypeInput)
        return kIOReturnError;
    
    if (!parent && digitiser.native)
        return kIOReturnNoDevice;
    
    if (!digitiser.transducers) {
        digitiser.transducers = OSArray::withCapacity(4);
        if (!digitiser.transducers)
            return kIOReturnError;
    }

    for (index=0, count = digitiser.transducers->getCount(); index < count; index++) {
        VoodooI2CDigitiserTransducer* temp_transducer;
        
        temp_transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(index));
        if (!temp_transducer)
            continue;

        if (temp_transducer->collection != parent)
            continue;

        transducer = temp_transducer;
        break;
    }

    if (!transducer) {
        DigitiserTransducuerType type = kDigitiserTransducerStylus;
        
        if (parent) {
            switch (parent->getUsage()) {
                case kHIDUsage_Dig_Puck:
                    type = kDigitiserTransducerPuck;
                    break;
                case kHIDUsage_Dig_Finger:
                case kHIDUsage_Dig_TouchScreen:
                case kHIDUsage_Dig_TouchPad:
                    type = kDigitiserTransducerFinger;
                    break;
                default:
                    break;
            }
        }

        if (type == kDigitiserTransducerStylus)
            transducer= VoodooI2CDigitiserStylus::transducer(type, parent);
        else
            transducer = VoodooI2CDigitiserTransducer::transducer(type, parent);

        if (!transducer)
            return kIOReturnError;
    
        digitiser.transducers->setObject(transducer);
    }

    if (should_calibrate)
        calibrateJustifiedPreferredStateElement(element, absolute_axis_removal_percentage);

    transducer->elements->setObject(element);

    return kIOReturnSuccess;
}
    
IOReturn VoodooI2CMultitouchHIDEventDriver::parseElements() {
    OSArray*   pending_elements         = NULL;
    OSArray*   pending_button_elements   = NULL;
    int index, count;

    supported_elements = hid_interface->createMatchingElements();

    if (!supported_elements)
        return kIOReturnNotFound;

    supported_elements->retain();

    for (index=0, count = supported_elements->getCount(); index < count; index++ ) {
        IOHIDElement* element = NULL;
        
        element = OSDynamicCast(IOHIDElement, supported_elements->getObject(index));
        if (!element)
            continue;
        
        if (element->getType() == kIOHIDElementTypeCollection)
            continue;
        
        if ( element->getUsage() == 0 )
            continue;
        
        if (parseDigitizerElement(element) != kIOReturnSuccess)
            continue;
        
        if (element->getUsagePage() == kHIDPage_Button) {
            IOHIDElement* parent = element;

            while ((parent = parent->getParentElement()) != NULL) {
                if (parent->getUsagePage() == kHIDPage_Consumer)
                    break;
            }
            if (parent != NULL)
                continue;

            if (!pending_button_elements) {
                pending_button_elements = OSArray::withCapacity(4);
                if (!pending_button_elements)
                    return kIOReturnError;
            }

            pending_button_elements->setObject(element);
            continue;
        }
        
        if (!pending_elements) {
            pending_elements = OSArray::withCapacity(4);
            if (!pending_elements) {
                return kIOReturnError;
            }
        }
        
        pending_elements->setObject(element);
    }

    if (pending_elements) {
        for (index = 0, count=pending_elements->getCount(); index < count; index++ ) {
            IOHIDElement* element = NULL;

            element = OSDynamicCast(IOHIDElement, pending_elements->getObject(index));
            if (!element)
                continue;

            parseDigitizerTransducerElement(element);
        }
    }

    if (pending_button_elements) {
        for (index = 0, count = pending_button_elements->getCount(); index < count; index++) {
            IOHIDElement* element = NULL;

            element = OSDynamicCast(IOHIDElement, pending_button_elements->getObject(index));
            if (!element)
                continue;
            
            if (digitiser.transducers && digitiser.transducers->getCount()) {
                VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(0));
                if (transducer)
                    transducer->elements->setObject(element);
            }
        }
    }

    processDigitizerElements();
    setDigitizerProperties();

    OSSafeReleaseNULL(pending_button_elements);
    OSSafeReleaseNULL(pending_elements);

    if (!digitiser.transducers->getCount())
        return kIOReturnNoDevice;

    return kIOReturnSuccess;
}

void VoodooI2CMultitouchHIDEventDriver::processDigitizerElements() {
    OSArray* new_transducers;
    OSArray* orphaned_elements;
    VoodooI2CDigitiserTransducer* root_transducer = NULL;
    UInt32 index, count;

    if (!digitiser.transducers)
        return;
    
    new_transducers = OSArray::withCapacity(4);
    if (!new_transducers)
        return;

    orphaned_elements = OSArray::withCapacity(4);
    if (!orphaned_elements)
        return;

    // Let's check for transducer validity. If there isn't an X axis, odds are
    // this transducer was created due to a malformed descriptor.  In this case,
    // let's collect the orphansed elements and insert them into the root transducer.

    for (index = 0, count = digitiser.transducers->getCount(); index < count; index++) {
        OSArray* pending_elements;
        VoodooI2CDigitiserTransducer* transducer;
        bool valid = false;
        UInt32 element_index, element_count;

        transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, digitiser.transducers->getObject(index));
        if (!transducer)
            continue;

        if (!transducer->elements)
            continue;

        pending_elements = OSArray::withCapacity(4);
        if (!pending_elements)
            continue;

        for (element_index = 0, element_count = transducer->elements->getCount(); element_index < element_count; element_index++) {
            IOHIDElement* element = OSDynamicCast(IOHIDElement, transducer->elements->getObject(element_index));

            if (!element)
                continue;

            if (element->getUsagePage()==kHIDPage_GenericDesktop && element->getUsage()==kHIDUsage_GD_X)
                valid = true;
            
            pending_elements->setObject(element);
        }
        
        if (valid) {
            new_transducers->setObject(transducer);

            if (!root_transducer)
                root_transducer = transducer;
        } else {
            orphaned_elements->merge(pending_elements);
        }

        if (pending_elements)
            pending_elements->release();
    }
    
    OSSafeReleaseNULL(digitiser.transducers);
    
    if (new_transducers->getCount()) {
    
        digitiser.transducers = new_transducers;
        digitiser.transducers->retain();
    
        if (root_transducer) {
            for (index = 0, count = orphaned_elements->getCount(); index < count; index++) {
            IOHIDElement* element = OSDynamicCast(IOHIDElement, orphaned_elements->getObject(index));

            if (!element)
                continue;

            root_transducer->elements->setObject(element);
            }
        }

        setProperty("SupportsInk", 1, 32);
    }

    if (orphaned_elements)
        orphaned_elements->release();

    if (new_transducers)
        new_transducers->release();

    return;
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

    properties->setObject("Contact Count Element", contact_count_element);
    properties->setObject("Input Mode Element", input_mode_element);
    properties->setObject("Contact Count Maximum  Element", contact_count_max_element);
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
