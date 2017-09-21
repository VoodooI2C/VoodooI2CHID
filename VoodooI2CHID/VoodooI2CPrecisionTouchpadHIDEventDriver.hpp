//
//  VoodooI2CPrecisionTouchpadHIDEventDriver.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 21/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CPrecisionTouchpadHIDEventDriver_hpp
#define VoodooI2CPrecisionTouchpadHIDEventDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include "VoodooI2CMultitouchHIDEventDriver.hpp"

#define kHIDUsage_Dig_Surface_Switch 0x57
#define kHIDUsage_Dig_Button_Switch 0x58

#define INPUT_MODE_MOUSE 0x00
#define INPUT_MODE_TOUCHPAD 0x03

typedef struct __attribute__((__packed__)) {
    UInt8 value;
    UInt8 reserved;
} VoodooI2CPrecisionTouchpadFeatureReport;

class VoodooI2CPrecisionTouchpadHIDEventDriver : public VoodooI2CMultitouchHIDEventDriver {
  OSDeclareDefaultStructors(VoodooI2CPrecisionTouchpadHIDEventDriver);

 public:
    bool handleStart(IOService* provider);
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice);

 protected:
 private:
    void enterPrecisionTouchpadMode();
};


#endif /* VoodooI2CPrecisionTouchpadHIDEventDriver_hpp */
