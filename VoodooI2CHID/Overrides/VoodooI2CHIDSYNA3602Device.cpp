//
//  VoodooI2CHIDSYNA3602Device.cpp
//  VoodooI2CHID
//
//  Created by Alexandre Daoud on 10/08/2019.
//  Copyright © 2019 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CHIDSYNA3602Device.hpp"

#define super VoodooI2CHIDDeviceOverride
OSDefineMetaClassAndStructors(VoodooI2CHIDSYNA3602Device, VoodooI2CHIDDeviceOverride);

bool VoodooI2CHIDSYNA3602Device::init(OSDictionary* properties) {
    if (!super::init(properties))
        return false;

    UInt8 descriptor_buffer[] = {
        0x1e, 0x00,                  /* Length of descriptor                 */
        0x00, 0x01,                  /* Version of descriptor                */
        0xdb, 0x01,                  /* Length of report descriptor          */
        0x21, 0x00,                  /* Location of report descriptor        */
        0x24, 0x00,                  /* Location of input report             */
        0x1b, 0x00,                  /* Max input report length              */
        0x25, 0x00,                  /* Location of output report            */
        0x11, 0x00,                  /* Max output report length             */
        0x22, 0x00,                  /* Location of command register         */
        0x23, 0x00,                  /* Location of data register            */
        0x11, 0x09,                  /* Vendor ID                            */
        0x88, 0x52,                  /* Product ID                           */
        0x06, 0x00,                  /* Version ID                           */
        0x00, 0x00, 0x00, 0x00       /* Reserved                             */
    };

    memcpy(&hid_descriptor_override, descriptor_buffer, sizeof(descriptor_buffer));

    report_descriptor_override = reinterpret_cast<UInt8*>(IOMalloc(sizeof(report_override_buffer)));
    memcpy(report_descriptor_override, report_override_buffer, sizeof(report_override_buffer));

    return true;
}

void VoodooI2CHIDSYNA3602Device::free() {
    IOFree(report_descriptor_override, sizeof(report_override_buffer));

    super::free();
}
