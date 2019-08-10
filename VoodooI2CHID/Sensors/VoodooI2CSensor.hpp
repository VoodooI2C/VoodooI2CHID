//
//  VoodooI2CSensor.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 26/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CSensor_hpp
#define VoodooI2CSensor_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/IOBufferMemoryDescriptor.h>

#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDUsageTables.h>

#include "VoodooI2CSensorsConstants.h"

typedef struct __attribute__((__packed__)) {
    UInt8 value;
    UInt8 reserved;
} VoodooI2CSensorFeatureReport;

class VoodooI2CSensorHubEventDriver;

class VoodooI2CSensor : public IOService {
  OSDeclareDefaultStructors(VoodooI2CSensor);

 public:
    IOHIDElement* element;

    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice);
    bool start(IOService* provider);
    void stop(IOService* provider);
    
    virtual void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id);
    static VoodooI2CSensor* withElement(IOHIDElement* element, IOService* event_driver);

 protected:
    bool awake;
    IOReturn changeState(IOHIDElement* state_element, UInt16 state_usage);
    VoodooI2CSensorHubEventDriver* event_driver;
    
    IOHIDElement* power_state;
    UInt32 current_power_state;
    
    
    IOHIDElement* reporting_state;
    UInt32 current_reporting_state;

    static UInt8 findPropertyIndex(IOHIDElement* element, UInt16 usage);
    UInt32 getElementValue(IOHIDElement* element);
    void setElementValue(IOHIDElement* element, UInt32 value);
};


#endif /* VoodooI2CSensor_hpp */
