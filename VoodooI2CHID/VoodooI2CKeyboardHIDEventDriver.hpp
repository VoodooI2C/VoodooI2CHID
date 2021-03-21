//
//  VoodooI2CKeyboardHIDEventDriver.hpp
//  VoodooI2CHID
//
//  Created by 夏尚宁 on 2021/3/17.
//  Copyright © 2021 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CKeyboardHIDEventDriver_hpp
#define VoodooI2CKeyboardHIDEventDriver_hpp

// hack to prevent IOHIDEventDriver from loading when
// we include IOHIDEventService

#define _IOKIT_HID_IOHIDEVENTDRIVER_H

#include <libkern/OSBase.h>

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/hid/IOHIDEvent.h>
#include <IOKit/hid/IOHIDEventService.h>
#include <IOKit/hid/IOHIDEventTypes.h>
#include <IOKit/hidsystem/IOHIDTypes.h>
#include <IOKit/hid/IOHIDPrivateKeys.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDDevice.h>

#include "../../../Dependencies/helpers.hpp"

// Message types defined by ApplePS2Keyboard
enum {
    // from keyboard to mouse/touchpad
    kKeyboardSetTouchStatus = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
    kKeyboardGetTouchStatus = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)
    kKeyboardKeyPressTime = iokit_vendor_specific_msg(110)      // notify of timestamp a non-modifier key was pressed (data is uint64_t*)
};

/* Implements an HID Event Driver for HID devices that expose a digitiser usage page.
 *
 * The members of this class are responsible for parsing, processing and interpreting digitiser-related HID objects.
 */

class EXPORT VoodooI2CKeyboardHIDEventDriver : public IOHIDEventService {
  OSDeclareDefaultStructors(VoodooI2CKeyboardHIDEventDriver);

 public:
    struct {
            OSArray *           elements;
            UInt8               bootMouseData[4];
            bool                appleVendorSupported;
    } keyboard;

    /* Notification that a provider has been terminated, sent after recursing up the stack, in leaf-to-root order.
     * @options The terminated provider of this object.
     * @defer If there is pending I/O that requires this object to persist, and the provider is not opened by this object set defer to true and call the IOService::didTerminate() implementation when the I/O completes. Otherwise, leave defer set to its default value of false.
     *
     * @return *true*
     */

    bool didTerminate(IOService* provider, IOOptionBits options, bool* defer) override;

    /*Gets the latest value of an element by issuing a getReport request to the
     * device. Necessary due to changes between 10.11 and 10.12.
     * @element The element whose vaue is to be updated
     *
     * @return The new value of the element
     */
    
    UInt32 getElementValue(IOHIDElement* element);
    
    const char* getProductName();
    
    /* Called during the interrupt routine to handle an interrupt report
     * @timestamp The timestamp of the interrupt report
     * @report A buffer containing the report data
     * @report_type The type of HID report
     * @report_id The report ID of the interrupt report
     */

    virtual void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id);

    /* Called during the start routine to set up the HID Event Driver
     * @provider The <IOHIDInterface> object which we have matched against.
     *
     * This function is reponsible for opening a client connection with the <IOHIDInterface> provider and for publishing
     * a multitouch interface into the IOService plane.
     *
     * @return *true* on successful start, *false* otherwise
     */

    bool handleStart(IOService* provider) override;
    
    
    /* Parses a keyboard usage page element
     * @element The element to parse
     *
     * This function is reponsible for examining the child elements of a digitser elements to determine the
     * capabilities of the keyboard.
     *
     * @return *true* on successful parse, *false* otherwise
     */

    bool parseKeyboardElement(IOHIDElement* element);


    /* Parses all matched elements
     *
     * @return *kIOReturnSuccess* on successful parse, *kIOReturnNotFound* if the matched elements are not supported, *kIOReturnError* otherwise
     */

    IOReturn parseElements();

    
    void setKeyboardProperties();

    /* Called by the OS in order to notify the driver that the device should change power state
     * @whichState The power state the device is expected to enter represented by either
     *  *kIOPMPowerOn* or *kIOPMPowerOff*
     * @whatDevice The power management policy maker
     *
     * This function exists to be overriden by inherited classes should they need it.
     *
     * @return *kIOPMAckImplied* on succesful state change, *kIOReturnError* otherwise
     */

    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;

    /* Called during the stop routine to terminate the HID Event Driver
     * @provider The <IOHIDInterface> object which we have matched against.
     *
     * This function is reponsible for releasing the resources allocated in <start>
     */

    void handleStop(IOService* provider) override;

    /* Implemented to set a certain property
     * @provider The <IOHIDInterface> object which we have matched against.
     */

    bool start(IOService* provider) override;
    
    /*
     * Called by ApplePS2Controller to notify of keyboard interactions
     * @type Custom message type in iokit_vendor_specific_msg range
     * @provider Calling IOService
     * @argument Optional argument as defined by message type
     *
     * @return kIOReturnSuccess if the message is processed
     */
    IOReturn message(UInt32 type, IOService* provider, void* argument) override;
    
    /*
     * Used to pass user preferences from user mode to the driver
     * @properties OSDictionary of configured properties
     *
     * @return kIOReturnSuccess if the properties are received successfully, otherwise kIOUnsupported
     */
    IOReturn setProperties(OSObject * properties) override;

 protected:
    const char* name;
    bool awake = true;
    IOHIDInterface* hid_interface;
    IOHIDDevice* hid_device;

 private:
    IOWorkLoop* work_loop;
    IOCommandGate* command_gate;
    
    uint64_t key_time = 0;
    
    OSSet* attached_hid_pointer_devices;
    
    IONotifier* usb_hid_publish_notify;     // Notification when an USB mouse HID device is connected
    IONotifier* usb_hid_terminate_notify; // Notification when an USB mouse HID device is disconnected
    
    IONotifier* bluetooth_hid_publish_notify; // Notification when a bluetooth HID device is connected
    IONotifier* bluetooth_hid_terminate_notify; // Notification when a bluetooth HID device is disconnected
    
    /*
     * IOServiceMatchingNotificationHandler (gated) to receive notification of addMatchingNotification registrations
     * @newService IOService object matching the criteria for the addMatchingNotification registration
     * @notifier IONotifier object for the notification registration
     */
    void notificationHIDAttachedHandlerGated(IOService * newService, IONotifier * notifier);
    
    /*
     * IOServiceMatchingNotificationHandler to receive notification of addMatchingNotification registrations
     * @refCon reference set when registering
     * @newService IOService object matching the criteria for the addMatchingNotification registration
     * @notifier IONotifier object for the notification registration
     */
    bool notificationHIDAttachedHandler(void * refCon, IOService * newService, IONotifier * notifier);
};

#endif /* VoodooI2CKeyboardHIDEventDriver_hpp */
