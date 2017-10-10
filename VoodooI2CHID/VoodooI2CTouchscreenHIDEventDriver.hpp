//
//  VoodooI2CTouchscreenHIDEventDriver.hpp
//  VoodooI2CHID
//
//  Created by blankmac on 9/30/17.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CTouchscreenHIDEventDriver_hpp
#define VoodooI2CTouchscreenHIDEventDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOWorkLoop.h>

#include "VoodooI2CMultitouchHIDEventDriver.hpp"



class VoodooI2CTouchscreenHIDEventDriver : public VoodooI2CMultitouchHIDEventDriver {
    OSDeclareDefaultStructors(VoodooI2CTouchscreenHIDEventDriver);


public:
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) override;
    bool handleStart(IOService* provider);
    IOWorkLoop *workLoop;
    IOTimerEventSource *timerSource;

    int clicktick = 0;
    UInt16 buttons = 0;
    int hover = 0;
    SInt16 last_x = 0;
    SInt16 last_y = 0;
    SInt16 last_x_bounds = 0;
    SInt16 last_y_bounds = 0;
    
    bool rightclick = false;
    UInt16 compare_input_x = 0;
    UInt16 compare_input_y = 0;
    int compare_input_counter = 0;
    
    bool checkStylus(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event);
    void checkFingerTouch(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event);
    void fingerLift();

protected:
private:

};
#endif /* VoodooI2CTouchscreenHIDEventDriver_hpp */
