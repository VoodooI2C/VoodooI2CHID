//
//  VoodooI2CDeviceOrientationSensor.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 26/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CDeviceOrientationSensor_hpp
#define VoodooI2CDeviceOrientationSensor_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include "VoodooI2CSensor.hpp"

typedef struct {
    SInt16 x;
    SInt16 y;
    SInt16 z;
    SInt16 w;
} VoodooI2CQuaternion;

class EXPORT VoodooI2CDeviceOrientationSensor : public VoodooI2CSensor {
  OSDeclareDefaultStructors(VoodooI2CDeviceOrientationSensor);

 public:
    bool start(IOService* provider) override;
    static VoodooI2CSensor* withElement(IOHIDElement* element, IOService* event_driver);

    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) override;

 private:
    IOHIDElement* quaternion;
};

#endif /* VoodooI2CDeviceOrientationSensor_hpp */
