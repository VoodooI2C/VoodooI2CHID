//
//  VoodooI2CStylusHIDEventDriver.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 23/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CStylusHIDEventDriver.hpp"

#define super VoodooI2CTouchscreenHIDEventDriver
OSDefineMetaClassAndStructors(VoodooI2CStylusHIDEventDriver, VoodooI2CTouchscreenHIDEventDriver);

bool VoodooI2CStylusHIDEventDriver::init(OSDictionary* properties) {
    if (!super::init(properties))
        return false;
    
    should_have_interface = false;
    
    return true;
}

void VoodooI2CStylusHIDEventDriver::handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) {
    if (!readyForReports() || report_type != kIOHIDReportTypeInput)
        return;
    
    digitiser.current_report = 1;
    digitiser.current_contact_count = 1;
    digitiser.report_count = 1;
    
    handleDigitizerReport(timestamp, report_id);
    
    VoodooI2CMultitouchEvent event;

    event.contact_count = 1;
    event.transducers = digitiser.transducers;
    
    checkStylus(timestamp, event);
}
