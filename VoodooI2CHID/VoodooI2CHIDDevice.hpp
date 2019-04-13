//
//  VoodooI2CHIDDevice.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDDevice_hpp
#define VoodooI2CHIDDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include "../../../Dependencies/helpers.hpp"

#define INTERRUPT_SIMULATOR_TIMEOUT 5

#define I2C_HID_PWR_ON  0x00
#define I2C_HID_PWR_SLEEP 0x01

typedef union {
    UInt8 data[4];
    struct __attribute__((__packed__)) cmd {
        UInt16 reg;
        UInt8 report_type_id;
        UInt8 opcode;
    } c;
} VoodooI2CHIDDeviceCommand;

typedef struct __attribute__((__packed__)) {
    UInt16 wHIDDescLength;
    UInt16 bcdVersion;
    UInt16 wReportDescLength;
    UInt16 wReportDescRegister;
    UInt16 wInputRegister;
    UInt16 wMaxInputLength;
    UInt16 wOutputRegister;
    UInt16 wMaxOutputLength;
    UInt16 wCommandRegister;
    UInt16 wDataRegister;
    UInt16 wVendorID;
    UInt16 wProductID;
    UInt16 wVersionID;
    UInt32 reserved;
} VoodooI2CHIDDeviceHIDDescriptor;

class VoodooI2CDeviceNub;

/* Implements an I2C-HID device as specified by Microsoft's protocol in the following document: http://download.microsoft.com/download/7/D/D/7DD44BB7-2A7A-4505-AC1C-7227D3D96D5B/hid-over-i2c-protocol-spec-v1-0.docx
 *
 * The members of this class are responsible for issuing I2C-HID commands via the device API as well as interacting with OS X's HID stack.
 */


class VoodooI2CHIDDevice : public IOHIDDevice {
  OSDeclareDefaultStructors(VoodooI2CHIDDevice);

 public:
    const char* name;

    /* Initialises a <VoodooI2CHIDDevice> object
     * @properties Contains the properties of the matched provider
     *
     * This is the first function called during the load routine and
     * allocates the memory needed for a <VoodooI2CHIDDevice> object.
     *
     * @return *true* upon successful initialisation, *false* otherwise
     */

    virtual bool init(OSDictionary* properties);

    /* Frees the <VoodooI2CHIDDevice> object
     *
     * This is the last function called during the unload routine and
     * frees the memory allocated in <init>.
     */

    virtual void free();

    /*
     * Issues an I2C-HID command to get the HID descriptor from the device.
     *
     * @return *kIOReturnSuccess* on sucessfully getting the HID descriptor, *kIOReturnIOError* if the request failed, *kIOReturnInvalid* if the descriptor is invalid
     */
    IOReturn getHIDDescriptor();

    /*
     * Gets the HID descriptor address by evaluating the device's '_DSM' method in the ACPI tables
     *
     * @return *kIOReturnSuccess* on sucessfully getting the HID descriptor address, *kIOReturnNotFound* if neither the '_DSM' method nor the '_XDSM' was found, *kIOReturnInvalid* if the address is invalid
     */

    IOReturn getHIDDescriptorAddress();
    
    IOReturn getReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options);

    /* Probes the candidate I2C-HID device to see if this driver can indeed drive it
     * @provider The provider which we have matched against
     * @score    Probe score as specified in the matched personality
     *
     * This function is responsible for querying the ACPI tables for the HID descriptor address and then
     * requesting the HID descriptor from the device.
     *
     * @return A pointer to this instance of VoodooI2CHID upon succesful probe, else NULL
     */

    VoodooI2CHIDDevice* probe(IOService* provider, SInt32* score);

    /* Run during the <IOHIDDevice::start> routine
     * @provider The provider which we have matched against
     *
     * We override <IOHIDDevice::handleStart> in order to allocate the work loop and resources
     * so that <IOHIDDevice> code is run synchronously with <VoodooI2CHID> code.
     *
     * @return *true* upon successful start, *false* otherwise
     */

    bool handleStart(IOService* provider);
    
    void simulateInterrupt(OSObject* owner, IOTimerEventSource* timer);
    
    /* Sets a few properties that are needed after <IOHIDDevice> finishes starting
     * @provider The provider which we have matched against
     *
     * @return *true* upon successful start, *false* otherwise
     */
    
    bool start(IOService* provider);

    /* Stops the I2C-HID Device
     * @provider The provider which we have matched against
     *
     * This function is called before <free> and is responsible for deallocating the resources
     * that were allocated in <start>.
     */

    void stop(IOService* provider);

    /* Create and return a new memory descriptor that describes the report descriptor for the HID device
     * @descriptor Pointer to the memory descriptor returned. This memory descriptor will be released by the caller.
     *
     * @return *kIOReturnSuccess* on success, or an error return otherwise.
     */

    IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;

    /* Returns a number object that describes the vendor ID of the HID device.
     *
     * @return A number object. The caller must decrement the retain count on the object returned.
     */

    OSNumber* newVendorIDNumber() const override;

    /* Returns a number object that describes the product ID of the HID device.
     *
     * @return A number object. The caller must decrement the retain count on the object returned.
     */

    OSNumber* newProductIDNumber() const override;

    /* Returns a number object that describes the version number of the HID device.
     *
     * @return A number object. The caller must decrement the retain count on the object returned.
     */

    OSNumber* newVersionNumber() const override;

    /* Returns a string object that describes the transport layer used by the HID device.
     *
     * @return A string object. The caller must decrement the retain count on the object returned.
     */

    OSString* newTransportString() const override;

    /* Returns a string object that describes the manufacturer of the HID device.
     *
     * @return A string object. The caller must decrement the retain count on the object returned.
     */

    OSString* newManufacturerString() const override;

 protected:
    bool awake;
    bool read_in_progress;
    IOWorkLoop* work_loop;
    
    IOReturn resetHIDDeviceGated();

    /* Issues an I2C-HID reset command.
     *
     * @return *kIOReturnSuccess* on successful reset, *kIOReturnTimeout* otherwise
     */

    IOReturn resetHIDDevice();

    /* Issues an I2C-HID power state command.
     * @state The power state that the device should enter
     *
     * @return *kIOReturnSuccess* on successful power state change, *kIOReturnTimeout* otherwise
     */

    IOReturn setHIDPowerState(VoodooI2CState state);

    /* Called by the OS's power management stack to notify the driver that it should change the power state of the deice
     * @whichState The power state the device is expected to enter represented by either
     *  *kIOPMPowerOn* or *kIOPMPowerOff*
     * @whatDevice The power management policy maker
     *
     *
     * @return *kIOPMAckImplied* on succesful state change, *kIOReturnError* otherwise
     */

    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice);

    /* Issues an I2C-HID set report command.
     * @report The report data to be sent to the device
     * @reportType The type of HID report to be sent
     * @options Options for the report, the first two bytes are the report ID
     *
     * @return *kIOReturnSuccess* on successful power state change, *kIOReturnTimeout* otherwise
     */

    IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options);

 private:
    IOACPIPlatformDevice* acpi_device;
    VoodooI2CDeviceNub* api;
    IOCommandGate* command_gate;
    UInt16 hid_descriptor_register;
    VoodooI2CHIDDeviceHIDDescriptor* hid_descriptor;
    IOTimerEventSource* interrupt_simulator;
    IOInterruptEventSource* interrupt_source;
    bool ready_for_input;
    bool* reset_event;

    /* Queries the I2C-HID device for an input report
     *
     * This function is called from the interrupt handler in a new thread. It is thus not called from interrupt context.
     */

    void getInputReport();
    
    IOWorkLoop* getWorkLoop();

    /*
    * This function is called when the I2C-HID device asserts its interrupt line.
    */
    
    void interruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);

    /* Releases resources allocated in <start>
     *
     * This function is called during a graceful exit from <start> and during
     * execution of <stop> in order to release resources retained by <start>.
     */

    void releaseResources();
};


#endif /* VoodooI2CHIDDevice_hpp */
