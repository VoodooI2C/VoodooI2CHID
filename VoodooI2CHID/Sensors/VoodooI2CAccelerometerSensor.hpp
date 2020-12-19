//
//  VoodooI2CAccelerometerSensor.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 28/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CAccelerometerSensor_hpp
#define VoodooI2CAccelerometerSensor_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/graphics/IODisplay.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/graphics/IOGraphicsTypes.h>

#include "VoodooI2CSensor.hpp"

#define kIOFBTransformKey               "IOFBTransform"

enum {
    kIOFBSetTransform = 0x00000400,
};

enum {
    kIOScaleRotateFlat = 0x00000070
};

class EXPORT VoodooI2CAccelerometerSensor : public VoodooI2CSensor {
  OSDeclareDefaultStructors(VoodooI2CAccelerometerSensor);

 public:
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor* report, IOHIDReportType report_type, UInt32 report_id) override;
    void rotateDevice(IOOptionBits rotation_state);
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    bool start(IOService* provider) override;
    static VoodooI2CSensor* withElement(IOHIDElement* sensor_element, IOService* event_driver);

 protected:
 private:
    IOFramebuffer* active_framebuffer;
    UInt8 current_rotation = kIOScaleRotate0;
    
    
    IOHIDElement* change_sensitivity;
    IOHIDElement* x_axis;
    IOHIDElement* y_axis;
    IOHIDElement* z_axis;

    IOFramebuffer* getFramebuffer();
};


#endif /* VoodooI2CAccelerometerSensor_hpp */
