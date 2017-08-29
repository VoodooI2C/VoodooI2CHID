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
#include "../../../Dependencies/helpers.hpp"

typedef union {
    UInt8 data[4];
    struct __attribute__((__packed__)) cmd {
        UInt16 regiser;
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

class VoodooI2CHIDDevice : public IOHIDDevice {
  OSDeclareDefaultStructors(VoodooI2CHIDDevice);

 public:
    bool attach(IOService* provider);
    void detach(IOService* provider);
    bool init(OSDictionary* properties);
    void free();
    IOReturn getHIDDescriptor();
    IOReturn getHIDDescriptorAddress();
    VoodooI2CHIDDevice* probe(IOService* provider, SInt32* score);
    bool start(IOService* provider);
    void stop(IOService* provider);

    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;
    virtual OSNumber* newVendorIDNumber() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSNumber* newVersionNumber() const override;
    virtual OSString* newTransportString() const override;
    virtual OSString* newManufacturerString() const override;
    virtual OSNumber* newPrimaryUsageNumber() const override;
    virtual OSNumber* newPrimaryUsagePageNumber() const override;

 protected:
 private:
    IOACPIPlatformDevice* acpi_device;
    VoodooI2CDeviceNub* api;
    UInt16 hid_descriptor_register;
    VoodooI2CHIDDeviceHIDDescriptor* hid_descriptor;
    const char* name;
};


#endif /* VoodooI2CHIDDevice_hpp */
