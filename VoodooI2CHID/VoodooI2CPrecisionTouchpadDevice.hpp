//
//  VoodooI2CPrecisionTouchpadDevice.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 31/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CPrecisionTouchpadDevice_hpp
#define VoodooI2CPrecisionTouchpadDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include "VoodooI2CHIDDevice.hpp"

#define INPUT_MODE_MOUSE 0x00
#define INPUT_MODE_TOUCHPAD 0x03

#define INPUT_CONFIG_REPORT_ID 3
#define TOUCHPAD_REPORT_ID 4

typedef struct __attribute__((__packed__)) {
    UInt8 mode;
    UInt8 reserved;
} VoodooI2CPrecisionTouchpadInputModeFeatureReport;

class VoodooI2CPrecisionTouchpadDevice : public VoodooI2CHIDDevice {
  OSDeclareDefaultStructors(VoodooI2CPrecisionTouchpadDevice);

 public:
   //  OSNumber* newPrimaryUsageNumber() const;
   // OSNumber* newPrimaryUsagePageNumber() const;
   bool start(IOService* provider);

 protected:
 private:
};


#endif /* VoodooI2CPrecisionTouchpadDevice_hpp */
