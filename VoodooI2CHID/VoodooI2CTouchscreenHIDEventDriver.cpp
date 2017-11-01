//
//  VoodooI2CTouchscreenHIDEventDriver.cpp
//  VoodooI2CHID
//
//  Created by blankmac on 9/30/17.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CTouchscreenHIDEventDriver.hpp"

#define super VoodooI2CMultitouchHIDEventDriver

OSDefineMetaClassAndStructors(VoodooI2CTouchscreenHIDEventDriver, VoodooI2CMultitouchHIDEventDriver);


// Override of VoodooI2CMultitouchHIDEventDriver

void VoodooI2CTouchscreenHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) {
    if (!readyForReports() || report_type != kIOHIDReportTypeInput)
        return;
    
    handleDigitizerReport(timestamp, report_id);
    VoodooI2CMultitouchEvent event;
    event.contact_count = contact_count_element->getValue();
    event.transducers = digitiser.transducers;
    
    //  Process single touch data
    if (!checkStylus(timestamp, event))
        checkFingerTouch(timestamp, event);
    
    //  Send multitouch information to the multitouch interface
    if (contact_count_element->getValue()>=2)
        super::handleInterruptReport(timestamp, report, report_type, report_id);
}

bool VoodooI2CTouchscreenHIDEventDriver::checkStylus(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event) {
    
    //  Check the current transducers for stylus operation, dispatch the pointer events and return true.
    //  At this time, Apple has removed all methods of handling additional information from the event driver.  Only x, y, buttonstate, and
    //  inrange are valid.
    
    for (int index = 0, count = event.transducers->getCount(); index < count; index++) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));

//        VoodooI2CDigitiserStylus* stylus = OSDynamicCast(VoodooI2CDigitiserStylus, transducer);


        if (transducer->type==kDigitiserTransducerStylus && transducer->in_range){
            
            
            
            IOGBounds boundary = {0, static_cast<SInt16>(transducer->logical_max_x), 0, static_cast<SInt16>(transducer->logical_max_y)};
            

//            if(stylus->barrel_switch.value()) {
//                buttons = 0x2;
//            } else {
//                buttons = stylus->tip_switch.value();
//            }
            
            buttons = transducer->tip_switch.value();
            
            dispatchAbsolutePointerEvent(timestamp, transducer->coordinates.x.value(), transducer->coordinates.y.value(), &boundary, buttons, transducer->in_range, 0, 0, 0);
            
//            dispatchAbsolutePointerEvent(timestamp, stylus->coordinates.x.value(), stylus->coordinates.y.value(), &boundary, buttons, stylus->in_range, 0, 0, 0);
            
            
            return true;
        }
        
    }
    
    return false;
}

void VoodooI2CTouchscreenHIDEventDriver::checkFingerTouch(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event) {
    
    //  If there is a finger touch event, decide if it is single or multitouch.
    
    for (int index = 0; index < contact_count_element->getValue(); index++) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));
        
        
        if (contact_count_element->getValue()>=2 && transducer->type==kDigitiserTransducerFinger) {
            
            //  Our finger event is multitouch reset clicktick and wait to be dispatched to the multitouch engines.
            
            clicktick = 0;
            
        } else if (transducer->type==kDigitiserTransducerFinger && contact_count_element->getValue()==1 && transducer->tip_switch){
                
                
                IOGBounds boundary = {0, static_cast<SInt16>(transducer->logical_max_x), 0, static_cast<SInt16>(transducer->logical_max_y)};

                //  We need the first couple of single touch events to be in hover mode.  In modes such as Mission Control, this allows us
                //  to select and drag windows vs just select and exit.  We are mimicking a cursor being moved into position prior to
                //  executing a drag movement.  There is little noticeable affect in other circumstances.  This also assists in transitioning
                //  between single / multitouch.
                
                if (clicktick<=2){
                    buttons = 0x0;
                    hover = 0x1;
                    clicktick++;
                } else {
                    buttons = transducer->tip_switch.value();
                    hover = transducer->in_range;
                }
                dispatchAbsolutePointerEvent(timestamp, transducer->coordinates.x.value(), transducer->coordinates.y.value(), &boundary, buttons, hover, 0, 0, 0);
                
                //  These coordinates allow us to send a finger lift event at watchdog timer timeout to the pointer's last position.
                //  This mechanism, in conjunction with only sending single touch events when the tip switch is set, is needed to exclude
                //  extraneous finger based pointer events.
                
                last_x = transducer->coordinates.x.value();
                last_y = transducer->coordinates.y.value();
                last_x_bounds = transducer->logical_max_x;
                last_y_bounds = transducer->logical_max_y;
                
                //  Begin long press right click routine.  Increasing compare_input_counter will lengthen the time until execution.
                
                UInt16 rtempx = transducer->coordinates.x.value();
                UInt16 rtempy = transducer->coordinates.y.value();

                if (!rightclick && contact_count_element->getValue()==1 && transducer->type==kDigitiserTransducerFinger && transducer->tip_switch) {
                    if (rtempx == compare_input_x && rtempy == compare_input_y) {
                        compare_input_counter = compare_input_counter + 1;
                        compare_input_x = rtempx;
                        compare_input_y = rtempy;
                        
                        if (compare_input_counter >= 120 && !rightclick) {
                            dispatchRelativePointerEvent(timestamp, 0, 0, 0x2);
                            compare_input_x = 0;
                            compare_input_y = 0;
                            compare_input_counter = 0;
                            rightclick = true;
                        }
                    }
                    else {
                        compare_input_x = rtempx;
                        compare_input_y = rtempy;
                        compare_input_counter = 0;
                    }
                }
                //  End long press right click routine.
                
                //  This timer serves to let us know when a finger based event is finished executing as well as let us
                // know to reset the clicktick counter.
                
                this->timerSource->setTimeoutMS(12);
            }
      }
}

void VoodooI2CTouchscreenHIDEventDriver::fingerLift() {
    
    //  Here we execute a single touch pointer lift event.  Finger based digitizer events have no in_range
    // component.  Greater accuracy / error rejection is achieved by filtering by tip_switch vs transducer id in the
    //  checkFingerTouch function, however, this has the side effect of not releasing the pointer.  This watchdog
    //  timer is needed regardless so the pointer release is best done here.
    
    clicktick=0;
    uint64_t now_abs;
    clock_get_uptime(&now_abs);
    IOGBounds boundary = {0, static_cast<SInt16>(last_x_bounds), 0, static_cast<SInt16>(last_y_bounds)};
    dispatchAbsolutePointerEvent(now_abs, last_x, last_y, &boundary, 0x0, 0x1, 0, 0, 0);

    //  If a right click has been executed, we reset our counter and ensure that pointer is not stuck in right
    //  click button down situation.
    
    if (rightclick) {
    rightclick = false;
        uint64_t now_abs;
        clock_get_uptime(&now_abs);
        dispatchRelativePointerEvent(now_abs, 0, 0, 0x0);

    }
    
}

bool VoodooI2CTouchscreenHIDEventDriver::handleStart(IOService* provider) {
    if (!super::handleStart(provider))
        return false;

    
    this->workLoop = getWorkLoop();
    if (!this->workLoop){
        IOLog("%s::Unable to get workloop\n", getName());
        stop(provider);
        return false;
    }
    
    this->workLoop->retain();

    this->timerSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &VoodooI2CTouchscreenHIDEventDriver::fingerLift));
    
    this->workLoop->addEventSource(this->timerSource);

    
    
    return true;
}
