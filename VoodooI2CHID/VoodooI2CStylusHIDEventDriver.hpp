//
//  VoodooI2CStylusHIDEventDriver.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 23/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CStylusHIDEventDriver_hpp
#define VoodooI2CStylusHIDEventDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include "VoodooI2CTouchscreenHIDEventDriver.hpp"

class EXPORT VoodooI2CStylusHIDEventDriver : public VoodooI2CTouchscreenHIDEventDriver {
  OSDeclareDefaultStructors(VoodooI2CStylusHIDEventDriver);

 public:
    bool init(OSDictionary* properties) override;
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) override;
};


#endif /* VoodooI2CStylusHIDEventDriver_hpp */
