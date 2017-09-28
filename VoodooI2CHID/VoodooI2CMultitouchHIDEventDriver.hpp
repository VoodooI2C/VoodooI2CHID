//
//  VoodooI2CMultitouchHIDEventDriver.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CMultitouchHIDEventDriver_hpp
#define VoodooI2CMultitouchHIDEventDriver_hpp

// hack to prevent IOHIDEventDriver from loading when
// we include IOHIDEventService

#define _IOKIT_HID_IOHIDEVENTDRIVER_H

#include <libkern/libkern/OSBase.h>

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/hid/IOHIDEvent.h>
#include <IOKit/hidevent/IOHIDEventService.h>
#include <IOKit/hid/IOHIDEventTypes.h>
#include <IOKit/hidsystem/IOHIDTypes.h>
#include <IOKit/hid/IOHIDPrivateKeys.h>
#include <IOKit/hid/IOHIDUsageTables.h>

#include "../../../Multitouch Support/VoodooI2CDigitiserStylus.hpp"
#include "../../../Multitouch Support/VoodooI2CMultitouchInterface.hpp"
#include "../../../Multitouch Support/MultitouchHelpers.hpp"

#include "../../../Dependencies/helpers.hpp"

#define kHIDUsage_Dig_Confidence kHIDUsage_Dig_TouchValid

class VoodooI2CMultitouchHIDEventDriver : public IOHIDEventService {
  OSDeclareDefaultStructors(VoodooI2CMultitouchHIDEventDriver);

 public:
    IOHIDElement* input_mode_element;
    IOHIDElement* contact_count_element;
    struct {
        OSArray*           transducers;
        bool               native;
    } digitiser;

    void calibrateJustifiedPreferredStateElement(IOHIDElement * element, SInt32 removalPercentage);
    bool didTerminate(IOService* provider, IOOptionBits options, bool* defer);
    void handleDigitizerReport(AbsoluteTime timestamp, UInt32 report_id);
    void handleDigitizerTransducerReport(VoodooI2CDigitiserTransducer* transducer, AbsoluteTime timestamp, UInt32 report_id);
    virtual void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id);
    bool handleStart(IOService* provider);
    IOReturn parseDigitizerElement(IOHIDElement* element);
    IOReturn parseDigitizerTransducerElement(IOHIDElement* element, IOHIDElement* parent);
    IOReturn parseElements();
    void processDigitizerElements();
    IOReturn publishMultitouchInterface();
    static inline void setButtonState(DigitiserTransducerButtonState* state, UInt32 bit, UInt32 value, AbsoluteTime timestamp);
    void setDigitizerProperties();
    virtual IOReturn setPowerState(unsigned long whichState, IOService* whatDevice);
    void handleStop(IOService* provider);
 protected:
    bool awake = true;
    IOHIDInterface* hid_interface;
 private:
    SInt32 absolute_axis_removal_percentage = 15;
    VoodooI2CMultitouchInterface* multitouch_interface;
    OSArray* supported_elements;
};


#endif /* VoodooI2CMultitouchHIDEventDriver_hpp */
