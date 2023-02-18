//
//  VoodooI2CSensorHubEventDriver.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CSensorHubEventDriver_hpp
#define VoodooI2CSensorHubEventDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDEventService.h>

#include "../../VoodooI2CHID/VoodooI2CHIDDevice.hpp"

class VoodooI2CSensor;
class VoodooI2CDeviceOrientationSensor;
class VoodooI2CAccelerometerSensor;

class EXPORT VoodooI2CSensorHubEventDriver : public IOHIDEventService {
  OSDeclareDefaultStructors(VoodooI2CSensorHubEventDriver);

 public:
    IOHIDDevice* hid_device;
    const char* name;
    
    bool handleStart(IOService* provider) override;
    void handleStop(IOService* provider) override;
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, UInt32 reportID);
    
    bool willTerminate(IOService* provider, IOOptionBits options) override;

 protected:
 private:
    IOHIDInterface* hid_interface;
    OSArray* supported_elements;
    
    OSArray* sensors;
    
    const char* getProductName();
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id);
    IOReturn parseSensorParent(IOHIDElement* parent);
};


#endif /* VoodooI2CSensorHubEventDriver_hpp */
