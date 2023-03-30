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
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/graphics/IODisplay.h>

#include "../../../Multitouch Support/VoodooI2CDigitiserStylus.hpp"
#include "../../../Multitouch Support/VoodooI2CMultitouchInterface.hpp"
#include "../../../Multitouch Support/MultitouchHelpers.hpp"


#include "VoodooI2CMultitouchHIDEventDriver.hpp"

#define FAT_FINGER_ZONE     1000000 // 1000^2
#define DOUBLE_CLICK_TIME   450 * 1000000
#define RIGHT_CLICK_TIME    500 * 1000000
#define FINGER_LIFT_DELAY   50
#define HOVER_TICKS         3

#define HOVER       0x0
#define LEFT_CLICK  0x1
#define RIGHT_CLICK 0x2
#define ERASE       0x4

/* Implements an HID Event Driver for touchscreen devices as well as stylus input.
 */

class EXPORT VoodooI2CTouchscreenHIDEventDriver : public VoodooI2CMultitouchHIDEventDriver {
    OSDeclareDefaultStructors(VoodooI2CTouchscreenHIDEventDriver);
    
 public:
    /* Checks the event contact count and if finger touches >= 2 are detected, the event is immediately dispatched
     * to the multitouch engine interface.  The 'else' convention used vs 'elseif' is intentional and results in
     * smoother gesture recognition and execution.  If single touch is detected, first the transducer is checked for stylus operation
     * and if false, the transducer is checked for finger touch.
     *
     * @inherit
     */
    void forwardReport(VoodooI2CMultitouchEvent event, AbsoluteTime timestamp) override;
    
    /* @inherit */
    bool handleStart(IOService* provider) override;
    
    /* @inherit */
    void handleStop(IOService* provider) override;
    
 protected:
    /* The transducer is checked for stylus operation and pointer event dispatched.  x,y,z & pressure information is
     * obtained in a logical format and converted to IOFixed variables.
     *
     * @timestamp The timestamp of the current event being processed
     *
     * @event The current event
     */

    bool checkStylus(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event);
    
    /*
     * Overriden to only check for kHIDUsage_Dig_TouchScreen
     */
    IOReturn parseElements(UInt32) override;

    /* Schedule a finger lift event
     */
    void scheduleLift();

 private:
    IOWorkLoop *work_loop;
    IOTimerEventSource *timer_source;
    
    IOFramebuffer* active_framebuffer;
    UInt8 current_rotation;
    
    /* transducer variables
     */
    
    UInt32 buttons = 0;
    UInt32 stylus_buttons = 0;
    IOFixed last_x = 0;
    IOFixed last_y = 0;
    IOFixed last_click_x = 0;
    IOFixed last_click_y = 0;
    IOFixed touch_start_x = 0;
    IOFixed touch_start_y = 0;
    UInt32 barrel_switch_offset = 0;
    UInt32 eraser_switch_offset = 0;
    SInt32 last_id = 0;
    
    /* handler variables
     */
    
    UInt32 click_tick = 0;
    bool right_click = false;
    bool moved_during_right_click = false;
    bool start_scroll = true;
    UInt64 touch_start_time = 0;
    UInt64 last_click_time = 0;
    
    /* The transducer is checked for singletouch finger based operation and the pointer event dispatched. This function
     * also handles a long-press, right-click function.
     *
     * @timestamp The timestamp of the current event being processed
     *
     * @event The current event
     * @return `true` if we got a finger touch event, `false` otherwise
     */
    bool checkFingerTouch(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event);
    
    /* Checks to see if the x and y coordinates need to be modified
     * to account for a rotation
     *
     * @x A pointer to the x coordinate
     * @y A pointer to the y coordinate
     *
     */

    void checkRotation(IOFixed* x, IOFixed* y);
    
    /* This timeout based function executes a singletouch finger based pointer lift event as well as ensures that the pointer is not
     * stuck in a 'right click' mode after the long-press right-click function has been triggered.
     */
    void fingerLift();
    
    IOFramebuffer* getFramebuffer();
    
    /* Resets the pointer to the current finger location when scrolling begins
     *
     * @timestamp The timestamp of the current event being processed
     *
     * @event The current event
     */
    
    void scrollPosition(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event);
};
#endif /* VoodooI2CTouchscreenHIDEventDriver_hpp */
