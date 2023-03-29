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

static uint64_t millis() {
    AbsoluteTime timestamp;
    uint64_t nanoseconds;
    clock_get_uptime(&timestamp);
    absolutetime_to_nanoseconds(timestamp, &nanoseconds);
    return nanoseconds / 1000000;
}

// Override of VoodooI2CMultitouchHIDEventDriver

bool VoodooI2CTouchscreenHIDEventDriver::checkFingerTouch(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event) {
    bool got_transducer = false;
    
    // If there is a finger touch event, decide if it is single or multitouch.
    
    for (int index = 0; index < digitiser.contact_count->getValue() + 1; index++) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));
        
        if (!transducer)
            return false;
        
        if (transducer->type == kDigitiserTransducerFinger && digitiser.contact_count->getValue() >= 2) {
            // Our finger event is multitouch reset clicktick and wait to be dispatched to the multitouch engines.
            
            click_tick = 0;
        }
        
        if (transducer->type == kDigitiserTransducerFinger && transducer->tip_switch.value()) {
            got_transducer = true;
            // Convert logical coordinates to IOFixed and Scaled;
            
            IOFixed x = ((transducer->coordinates.x.value() * 1.0f) / transducer->logical_max_x) * 65535;
            IOFixed y = ((transducer->coordinates.y.value() * 1.0f) / transducer->logical_max_y) * 65535;
            
            checkRotation(&x, &y);
            // Begin long press right click routine.  Increasing compare_input_counter check will lengthen the time until execution.
            
            UInt16 temp_x = x;
            UInt16 temp_y = y;
            
            if (!right_click && digitiser.contact_count->getValue() == 1 && transducer->type == kDigitiserTransducerFinger && transducer->tip_switch) {
                if (temp_x == compare_input_x && temp_y == compare_input_y) {
                    compare_input_counter = compare_input_counter + 1;
                    compare_input_x = temp_x;
                    compare_input_y = temp_y;
                    
                    if (compare_input_counter >= 120 && !right_click) {
                        compare_input_x = 0;
                        compare_input_y = 0;
                        compare_input_counter = 0;
                        right_click = true;
                    }
                } else {
                    compare_input_x = temp_x;
                    compare_input_y = temp_y;
                    compare_input_counter = 0;
                }
            }
            //  End long press right click routine.
            
            
            //  We need the first couple of single touch events to be in hover mode.  In modes such as Mission Control, this allows us
            //  to select and drag windows vs just select and exit.  We are mimicking a cursor being moved into position prior to
            //  executing a drag movement.  There is little noticeable affect in other circumstances.  This also assists in transitioning
            //  between single / multitouch.
            
            if (click_tick < 1) {
                buttons = HOVER;
                click_tick++;
            } else {
                buttons = transducer->tip_switch.value();
            }
            if (right_click)
                buttons = RIGHT_CLICK;
            
            // If we're clicking again where we just clicked, precisely position the pointer where it was before
            if (
                isCloseToLastInteraction(x, y) &&
                (millis() - last_click_time) <= DOUBLE_CLICK_TIME
            ) {
                x = last_click_x;
                y = last_click_y;
            }

            dispatchDigitizerEventWithTiltOrientation(timestamp, transducer->secondary_id, transducer->type, 0x1, buttons, x, y);

            if (buttons == HOVER) {
                IOLog("%s::Hover at %d, %d\n", getName(), x, y);
            }
            else if (buttons == LEFT_CLICK) {
                IOLog("%s::Left click at %d, %d\n", getName(), x, y);
            }
            else if (buttons == RIGHT_CLICK) {
                IOLog("%s::Right click at %d, %d\n", getName(), x, y);
            }

            // Track last ID and coordinates so that we can send the finger lift event after our watch dog timeout.
            last_x = x;
            last_y = y;
            last_id = transducer->secondary_id;

            //  This timer serves to let us know when a finger based event is finished executing as well as let us
            // know to reset the clicktick counter.
            
            scheduleLift();
        }
    }
    return got_transducer;
}

void VoodooI2CTouchscreenHIDEventDriver::checkRotation(IOFixed* x, IOFixed* y) {
    if (active_framebuffer) {
        if (current_rotation & kIOFBSwapAxes) {
            IOFixed old_x = *x;
            *x = *y;
            *y = old_x;
        }
        if (current_rotation & kIOFBInvertX)
            *x = 65535 - *x;
        if (current_rotation & kIOFBInvertY)
            *y = 65535 - *y;
    }
}

IOReturn VoodooI2CTouchscreenHIDEventDriver::parseElements(UInt32) {
    return super::parseElements(kHIDUsage_Dig_TouchScreen);
}

bool VoodooI2CTouchscreenHIDEventDriver::checkStylus(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event) {
    //  Check the current transducers for stylus operation, dispatch the pointer events and return true.
    //  At this time, Apple has removed all methods of handling additional information from the event driver.  Only x, y, buttonstate, and
    //  inrange are valid for macOS Sierra +.  10.11 still makes use of extended functions.
    
    for (int index = 0, count = event.transducers->getCount(); index < count; index++) {
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));

        if (transducer->type == kDigitiserTransducerStylus && transducer->in_range) {
            VoodooI2CDigitiserStylus* stylus = (VoodooI2CDigitiserStylus*)transducer;
            IOFixed x = ((stylus->coordinates.x.value() * 1.0f) / stylus->logical_max_x) * 65535;
            IOFixed y = ((stylus->coordinates.y.value() * 1.0f) / stylus->logical_max_y) * 65535;
            IOFixed z = ((stylus->coordinates.z.value() * 1.0f) / stylus->logical_max_z) * 65535;
            IOFixed stylus_pressure = ((stylus->tip_pressure.value() * 1.0f) /stylus->pressure_physical_max) * 65535;
            
            checkRotation(&x, &y);
            
            if (stylus->barrel_switch.value() != HOVER && stylus->barrel_switch.value() != RIGHT_CLICK && (stylus->barrel_switch.value()-barrel_switch_offset) != RIGHT_CLICK)
                barrel_switch_offset = stylus->barrel_switch.value();
            if (stylus->eraser.value() != HOVER && stylus->eraser.value() != RIGHT_CLICK && (stylus->eraser.value()-eraser_switch_offset) != ERASE)
                eraser_switch_offset = stylus->eraser.value();
            
            stylus_buttons = stylus->tip_switch.value();
            
            if (stylus->barrel_switch.value() == RIGHT_CLICK || (stylus->barrel_switch.value() - barrel_switch_offset) == RIGHT_CLICK) {
                stylus_buttons = RIGHT_CLICK;
            }
            
            if (stylus->eraser.value() == ERASE || (stylus->eraser.value() - eraser_switch_offset) == ERASE) {
                stylus_buttons = ERASE;
            }
            
            dispatchDigitizerEventWithTiltOrientation(timestamp, stylus->secondary_id, stylus->type, stylus->in_range, stylus_buttons, x, y, z, stylus_pressure, stylus->barrel_pressure.value(), stylus->azi_alti_orientation.twist.value(), stylus->tilt_orientation.x_tilt.value(), stylus->tilt_orientation.y_tilt.value());
            
            IOLog("%s::Stylus at %d, %d\n", getName(), x, y);

            return true;
        }
    }
    
    return false;
}

void VoodooI2CTouchscreenHIDEventDriver::fingerLift() {
    //  Here we execute a single touch pointer lift event.  Finger based digitizer events have no in_range
    // component.  Greater accuracy / error rejection is achieved by filtering by tip_switch vs transducer id in the
    //  checkFingerTouch function, however, this has the side effect of not releasing the pointer.  This watchdog
    //  timer is needed regardless so the pointer release is best done here.
    
    
    click_tick = 0;
    start_scroll = true;
    uint64_t now_abs;
    clock_get_uptime(&now_abs);

    dispatchDigitizerEventWithTiltOrientation(now_abs, last_id, kDigitiserTransducerFinger, 0x1, HOVER, last_x, last_y);
    IOLog("%s::Finger lift at %d, %d\n", getName(), last_x, last_y);
    
    //  If a right click has been executed, we reset our counter and ensure that pointer is not stuck in right
    //  click button down situation.
    
    if (right_click) {
        right_click = false;
    }
    
    last_click_time = millis();
    last_click_x = last_x;
    last_click_y = last_y;
}

IOFramebuffer* VoodooI2CTouchscreenHIDEventDriver::getFramebuffer() {
    IORegistryEntry* display = NULL;
    IOFramebuffer* framebuffer = NULL;
    
    OSDictionary *match = serviceMatching("IODisplay");
    OSIterator *iterator = getMatchingServices(match);

    if (iterator) {
        display = OSDynamicCast(IORegistryEntry, iterator->getNextObject());
        
        if (display) {
            IORegistryEntry *entry = display->getParentEntry(gIOServicePlane)->getParentEntry(gIOServicePlane);
            if (entry)
                framebuffer = reinterpret_cast<IOFramebuffer*>(entry->metaCast("IOFramebuffer"));
        }
        
        iterator->release();
    }
    
    OSSafeReleaseNULL(match);

    return framebuffer;
}

void VoodooI2CTouchscreenHIDEventDriver::forwardReport(VoodooI2CMultitouchEvent event, AbsoluteTime timestamp) {
    if (!active_framebuffer) {
        active_framebuffer = getFramebuffer();
        
        if (active_framebuffer) {
            active_framebuffer->retain();
        }
    }

    if (active_framebuffer) {
        OSNumber* number = OSDynamicCast(OSNumber, active_framebuffer->getProperty(kIOFBTransformKey));
        current_rotation = number->unsigned8BitValue() / 0x10;
        multitouch_interface->setProperty(kIOFBTransformKey, current_rotation, 8);
    }
    
    if (event.contact_count) {
        // Send multitouch information to the multitouch interface

        if (event.contact_count >= 2) {
            if (event.contact_count == 2 && start_scroll) {
                scrollPosition(timestamp, event);
            } else if (event.contact_count == 2 && !start_scroll) {
                scheduleLift();
            }

            multitouch_interface->handleInterruptReport(event, timestamp);
        } else {
            // Process single touch data
            if (!checkStylus(timestamp, event)) {
                if (!checkFingerTouch(timestamp, event))
                    multitouch_interface->handleInterruptReport(event, timestamp);
            }
        }
    }
}

bool VoodooI2CTouchscreenHIDEventDriver::handleStart(IOService* provider) {
    if (!super::handleStart(provider))
        return false;
    
    work_loop = getWorkLoop();
    if (!work_loop) {
        IOLog("%s::Unable to get workloop\n", getName());
        stop(provider);
        return false;
    }
    
    work_loop->retain();
    
    timer_source = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &VoodooI2CTouchscreenHIDEventDriver::fingerLift));
    
    if (!timer_source || work_loop->addEventSource(timer_source) != kIOReturnSuccess) {
        IOLog("%s::Could not add timer source to work loop\n", getName());
        return false;
    }
    
    active_framebuffer = getFramebuffer();
    if (active_framebuffer) {
        active_framebuffer->retain();
    }
    
    return true;
}

void VoodooI2CTouchscreenHIDEventDriver::handleStop(IOService* provider) {
    if (timer_source) {
        work_loop->removeEventSource(timer_source);
        OSSafeReleaseNULL(timer_source);
    }
    
    OSSafeReleaseNULL(work_loop);
    
    if (active_framebuffer) {
        OSSafeReleaseNULL(active_framebuffer);
    }
    
    super::handleStop(provider);
}

void VoodooI2CTouchscreenHIDEventDriver::scrollPosition(AbsoluteTime timestamp, VoodooI2CMultitouchEvent event) {
    if (start_scroll) {
        int index = 0;
        VoodooI2CDigitiserTransducer* transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));
        if (transducer->type == kDigitiserTransducerStylus) {
            index = 1;
            transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));
        }
        
        IOFixed x = ((transducer->coordinates.x.value() * 1.0f) / transducer->logical_max_x) * 65535;
        IOFixed y = ((transducer->coordinates.y.value() * 1.0f) / transducer->logical_max_y) * 65535;
        
        index++;
        transducer = OSDynamicCast(VoodooI2CDigitiserTransducer, event.transducers->getObject(index));
        
        IOFixed x2 = ((transducer->coordinates.x.value() * 1.0f) / transducer->logical_max_x) * 65535;
        IOFixed y2 = ((transducer->coordinates.y.value() * 1.0f) / transducer->logical_max_y) * 65535;
        
        IOFixed cursor_x = (x+x2)/2;
        IOFixed cursor_y = (y+y2)/2;
        
        checkRotation(&cursor_x, &cursor_y);
        
        dispatchDigitizerEventWithTiltOrientation(timestamp, transducer->secondary_id, transducer->type, 0x1, HOVER, cursor_x, cursor_y);
        IOLog("%s::Hover for scroll at %d, %d\n", getName(), cursor_x, cursor_y);

        last_x = cursor_x;
        last_y = cursor_y;
        last_id = transducer->secondary_id;
        
        start_scroll = false;
    }
    
    scheduleLift();
}

bool VoodooI2CTouchscreenHIDEventDriver::isCloseToLastInteraction(IOFixed x, IOFixed y) {
    return (
        abs(x - last_click_x) <= DOUBLE_CLICK_FAT_ZONE &&
        abs(y - last_click_y) <= DOUBLE_CLICK_FAT_ZONE
    );
}

void VoodooI2CTouchscreenHIDEventDriver::scheduleLift() {
    timer_source->cancelTimeout();
    timer_source->setTimeoutMS(FINGER_LIFT_DELAY);
}
