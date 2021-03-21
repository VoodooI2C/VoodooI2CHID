//
//  VoodooI2CKeyboardHIDEventDriver.cpp
//  VoodooI2CHID
//
//  Created by 夏尚宁 on 2021/3/17.
//  Copyright © 2021 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CKeyboardHIDEventDriver.hpp"
#include <IOKit/IOCommandGate.h>
#include <IOKit/hid/IOHIDInterface.h>
#include <IOKit/usb/USBSpec.h>
#include <IOKit/bluetooth/BluetoothAssignedNumbers.h>
#include <IOKit/IOLib.h>

#include <IOKit/hid/AppleHIDUsageTables.h>

#define SET_NUMBER(key, num) do { \
    tmpNumber = OSNumber::withNumber(num, 32); \
    if (tmpNumber) { \
        kbEnableEventProps->setObject(key, tmpNumber); \
        tmpNumber->release(); \
    } \
}while (0);

// constants for processing the special key input event
#define kHIDIncrVolume  0x01
#define kHIDDecrVolume  0x02
#define kHIDMute        0x04

#define GetReportType(type)                                             \
((type <= kIOHIDElementTypeInput_ScanCodes) ? kIOHIDReportTypeInput :   \
(type <= kIOHIDElementTypeOutput) ? kIOHIDReportTypeOutput :            \
(type <= kIOHIDElementTypeFeature) ? kIOHIDReportTypeFeature : -1)

#define super IOHIDEventService
OSDefineMetaClassAndStructors(VoodooI2CKeyboardHIDEventDriver, IOHIDEventService);

bool VoodooI2CKeyboardHIDEventDriver::didTerminate(IOService* provider, IOOptionBits options, bool* defer) {
    if (hid_interface)
        hid_interface->close(this);
    hid_interface = NULL;
    
    return super::didTerminate(provider, options, defer);
}

UInt32 VoodooI2CKeyboardHIDEventDriver::getElementValue(IOHIDElement* element) {
    IOHIDElementCookie cookie = element->getCookie();
    
    if (!cookie)
        return 0;
    
    hid_device->updateElementValues(&cookie);
    
    return element->getValue();
}

const char* VoodooI2CKeyboardHIDEventDriver::getProductName() {
    if (OSString* name = getProduct())
        return name->getCStringNoCopy();

    return "Keyboard HID Device";
}

void VoodooI2CKeyboardHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) {
    UInt32      volumeHandled   = 0;
    UInt32      volumeState     = 0;
    UInt32      index, count;
    
    if(!keyboard.elements)
        goto exit;
    
    for (index=0, count=keyboard.elements->getCount(); index<count; index++) {
        IOHIDElement* element = nullptr;
        AbsoluteTime  elementTimeStamp;
        UInt32        usagePage, usage, value, preValue;
        
        element = OSDynamicCast(IOHIDElement, keyboard.elements->getObject(index));
        if (!element)
            continue;
        
        if (element->getReportID() != report_id)
            continue;
        
        elementTimeStamp = element->getTimeStamp();
        if (CMP_ABSOLUTETIME(&timestamp, &elementTimeStamp) != 0)
            continue;
        
        preValue    = element->getValue(kIOHIDValueOptionsFlagPrevious) != 0;
        value       = element->getValue() != 0;
        
        if (value == preValue)
            continue;
        
        usagePage   = element->getUsagePage();
        usage       = element->getUsage();
        
        if (usagePage == kHIDPage_Consumer) {
            bool suppress = true;
            switch (usage) {
                case kHIDUsage_Csmr_VolumeIncrement:
                    volumeHandled   |= kHIDIncrVolume;
                    volumeState     |= (value) ? kHIDIncrVolume:0;
                    break;
                case kHIDUsage_Csmr_VolumeDecrement:
                    volumeHandled   |= kHIDDecrVolume;
                    volumeState     |= (value) ? kHIDDecrVolume:0;
                    break;
                case kHIDUsage_Csmr_Mute:
                    volumeHandled   |= kHIDMute;
                    volumeState     |= (value) ? kHIDMute:0;
                    break;
                default:
                    suppress = false;
                    break;
            }
            
            if (suppress)
                continue;
        }
        
        dispatchKeyboardEvent(timestamp, usagePage, usage, value);
    }
    
    // RY: Handle the case where Vol Increment, Decrement, and Mute are all down
    // If such an event occurs, it is likely that the device is defective,
    // and should be ignored.
    if ((volumeState != (kHIDIncrVolume|kHIDDecrVolume|kHIDMute)) &&
        (volumeHandled != (kHIDIncrVolume|kHIDDecrVolume|kHIDMute))) {
        // Volume Increment
        if (volumeHandled & kHIDIncrVolume)
            dispatchKeyboardEvent(timestamp, kHIDPage_Consumer, kHIDUsage_Csmr_VolumeIncrement, ((volumeState & kHIDIncrVolume) != 0));
        // Volume Decrement
        if (volumeHandled & kHIDDecrVolume)
            dispatchKeyboardEvent(timestamp, kHIDPage_Consumer, kHIDUsage_Csmr_VolumeDecrement, ((volumeState & kHIDDecrVolume) != 0));
        // Volume Mute
        if (volumeHandled & kHIDMute)
            dispatchKeyboardEvent(timestamp, kHIDPage_Consumer, kHIDUsage_Csmr_Mute, ((volumeState & kHIDMute) != 0));
    }
    
exit:
    return;
}

bool VoodooI2CKeyboardHIDEventDriver::handleStart(IOService* provider) {
    if(!super::handleStart(provider)) {
        return false;
    }
    
    hid_interface = OSDynamicCast(IOHIDInterface, provider);

    if (!hid_interface)
        return false;

    OSString* transport = hid_interface->getTransport();
    if (!transport)
        return false;
  
    if (strncmp(transport->getCStringNoCopy(), kIOHIDTransportUSBValue, sizeof(kIOHIDTransportUSBValue)) != 0)

        hid_interface->setProperty("VoodooI2CServices Supported", kOSBooleanTrue);

    hid_device = OSDynamicCast(IOHIDDevice, hid_interface->getParentEntry(gIOServicePlane));
    
    if (!hid_device)
        return false;
    
    name = getProductName();

    if (parseElements() != kIOReturnSuccess) {
        IOLog("%s::%s Could not parse multitouch elements\n", getName(), name);
        return false;
    }
    
    if (!hid_interface->open(this, 0, OSMemberFunctionCast(IOHIDInterface::InterruptReportAction, this, &VoodooI2CKeyboardHIDEventDriver::handleInterruptReport), NULL))
        return false;

    setKeyboardProperties();

    PMinit();
    hid_interface->joinPMtree(this);
    registerPowerDriver(this, VoodooI2CIOPMPowerStates, kVoodooI2CIOPMNumberPowerStates);

    return true;
}

void VoodooI2CKeyboardHIDEventDriver::handleStop(IOService* provider) {
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }

    OSSafeReleaseNULL(work_loop);

    PMstop();
    super::handleStop(provider);
}

bool VoodooI2CKeyboardHIDEventDriver::parseKeyboardElement(IOHIDElement * element) {
    UInt32 usagePage    = element->getUsagePage();
    UInt32 usage        = element->getUsage();
    bool   store        = false;
    
    if (!keyboard.elements) {
        keyboard.elements = OSArray::withCapacity(4);
        if(!keyboard.elements)
            goto exit;
    }
    
    switch (usagePage) {
        case kHIDPage_GenericDesktop:
            switch (usage) {
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
            if ((usage < kHIDUsage_KeyboardA) || (usage > kHIDUsage_KeyboardRightGUI))
                break;
            
            // This usage is used to let the OS know if a keyboard is in an enabled state where
            // user input is possible
            
            if (usage == kHIDUsage_KeyboardPower) {
                OSDictionary* kbEnableEventProps    = NULL;
                UInt32 value                        = 0;
                
                // To avoid problems with un-intentional clearing of the flag
                // we require this report to be a feature report so that the current
                // state can be polled if necessary
                
                if (element->getType() == kIOHIDElementTypeFeature) {
                    value = element->getValue(kIOHIDValueOptionsUpdateElementValues);
                    
                    kbEnableEventProps = OSDictionary::withCapacity(3);
                    if (!kbEnableEventProps)
                        break;
                    OSSafeReleaseNULL(kbEnableEventProps);
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

    
IOReturn VoodooI2CKeyboardHIDEventDriver::parseElements() {
    //Keyboard : Loop through all createMatchingElements() and parse KeyboardElements
        
    OSArray *elementArray = hid_interface->createMatchingElements();
    keyboard.appleVendorSupported = getProperty(kIOHIDAppleVendorSupported, gIOServicePlane);
    if (elementArray) {
        for (int i=0, count=elementArray->getCount(); i<count; i++) {
            IOHIDElement* element   = nullptr;
            
            element = OSDynamicCast(IOHIDElement, elementArray->getObject(i));
            if (!element)
                continue;
            
            if (element->getType() == kIOHIDElementTypeCollection)
                continue;
            
            if (element->getUsage() == 0)
                continue;
            
            if (parseKeyboardElement(element))
                continue;
        }
    }
    OSSafeReleaseNULL(elementArray);

    return kIOReturnSuccess;
}

void VoodooI2CKeyboardHIDEventDriver::setKeyboardProperties()
{
    OSDictionary *properties = OSDictionary::withCapacity(4);
    
    if (!properties)
        return;
    
    if (!keyboard.elements)
        return;
    
    properties->setObject(kIOHIDElementKey, keyboard.elements);
    
    setProperty("Keyboard", properties);
    
exit:
    OSSafeReleaseNULL(properties);
}

IOReturn VoodooI2CKeyboardHIDEventDriver::setPowerState(unsigned long whichState, IOService* whatDevice) {
    return kIOPMAckImplied;
}

bool VoodooI2CKeyboardHIDEventDriver::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    work_loop = getWorkLoop();
    
    if (!work_loop)
        return false;
    
    work_loop->retain();
    
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate) {
        return false;
    }
    work_loop->addEventSource(command_gate);

    setProperty("VoodooI2CServices Supported", kOSBooleanTrue);

    return true;
}

IOReturn VoodooI2CKeyboardHIDEventDriver::message(UInt32 type, IOService* provider, void* argument) {
    switch (type) {
        case kKeyboardKeyPressTime:
        {
            //  Remember last time key was pressed
            key_time = *((uint64_t*)argument);
#if DEBUG
            IOLog("%s::keyPressed = %llu\n", getName(), key_time);
#endif
            break;
        }
    }

    return kIOReturnSuccess;
}

void VoodooI2CKeyboardHIDEventDriver::notificationHIDAttachedHandlerGated(IOService * newService, IONotifier * notifier) {
    char path[256];
    int len = 255;
    memset(path, 0, len);
    newService->getPath(path, &len, gIOServicePlane);
    
    if (notifier == usb_hid_publish_notify) {
        IORegistryEntry* hid_child = OSDynamicCast(IORegistryEntry, newService->getChildEntry(gIOServicePlane));
        
        if (!hid_child)
            return;

        OSNumber* primary_usage_page = OSDynamicCast(OSNumber, hid_child->getProperty(kIOHIDPrimaryUsagePageKey));
        OSNumber* primary_usage= OSDynamicCast(OSNumber, hid_child->getProperty(kIOHIDPrimaryUsageKey));
        
        if (!primary_usage_page || !primary_usage)
            return;
        
        // ignore touchscreens

        if (primary_usage_page->unsigned8BitValue() != kHIDPage_Digitizer && primary_usage->unsigned8BitValue() != kHIDUsage_Dig_TouchScreen) {
            attached_hid_pointer_devices->setObject(newService);
            IOLog("%s: USB pointer HID device published: %s, # devices: %d\n", getName(), path, attached_hid_pointer_devices->getCount());
        }
    }
    
    if (notifier == usb_hid_terminate_notify) {
        attached_hid_pointer_devices->removeObject(newService);
        IOLog("%s: USB pointer HID device terminated: %s, # devices: %d\n", getName(), path, attached_hid_pointer_devices->getCount());
    }
    
    if (notifier == bluetooth_hid_publish_notify) {
        // Filter on specific CoD (Class of Device) bluetooth devices only
        OSNumber* propDeviceClass = OSDynamicCast(OSNumber, newService->getProperty("ClassOfDevice"));
        
        if (propDeviceClass != NULL) {
            long classOfDevice = propDeviceClass->unsigned32BitValue();
            
            long deviceClassMajor = (classOfDevice & 0x1F00) >> 8;
            long deviceClassMinor = (classOfDevice & 0xFF) >> 2;
            
            if (deviceClassMajor == kBluetoothDeviceClassMajorPeripheral) { // Bluetooth peripheral devices
                long deviceClassMinor1 = (deviceClassMinor) & 0x30;
                long deviceClassMinor2 = (deviceClassMinor) & 0x0F;
                
                if (deviceClassMinor1 == kBluetoothDeviceClassMinorPeripheral1Pointing || // Seperate pointing device
                    deviceClassMinor1 == kBluetoothDeviceClassMinorPeripheral1Combo) // Combo bluetooth keyboard/touchpad
                {
                    if (deviceClassMinor2 == kBluetoothDeviceClassMinorPeripheral2Unclassified || // Mouse
                        deviceClassMinor2 == kBluetoothDeviceClassMinorPeripheral2DigitizerTablet || // Magic Touchpad
                        deviceClassMinor2 == kBluetoothDeviceClassMinorPeripheral2DigitalPen) // Wacom Tablet
                    {
                        attached_hid_pointer_devices->setObject(newService);
                        IOLog("%s: Bluetooth pointer HID device published: %s, # devices: %d\n", getName(), path, attached_hid_pointer_devices->getCount());
                    }
                }
            }
        }
    }
    
    if (notifier == bluetooth_hid_terminate_notify) {
        attached_hid_pointer_devices->removeObject(newService);
        IOLog("%s: Bluetooth pointer HID device terminated: %s, # devices: %d\n", getName(), path, attached_hid_pointer_devices->getCount());
    }
}

bool VoodooI2CKeyboardHIDEventDriver::notificationHIDAttachedHandler(void * refCon, IOService * newService, IONotifier * notifier) {
    command_gate->runAction((IOCommandGate::Action)OSMemberFunctionCast(
                            IOCommandGate::Action, this,
                            &VoodooI2CKeyboardHIDEventDriver::notificationHIDAttachedHandlerGated),
                            newService, notifier);

    return true;
}
