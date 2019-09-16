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

    const UInt8 descriptor_buffer[] = {
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

    const UInt8 report_buffer[] = {
        0x05, 0x01,                  /* Usage Page (Desktop),                */
        0x09, 0x02,                  /* Usage (Mouse),                       */
        0xA1, 0x01,                  /* Collection (Application),            */
        0x85, 0x01,                  /*     Report ID (1),                   */
        0x09, 0x01,                  /*     Usage (Pointer),                 */
        0xA1, 0x00,                  /*     Collection (Physical),           */
        0x05, 0x09,                  /*         Usage Page (Button),         */
        0x19, 0x01,                  /*         Usage Minimum (01h),         */
        0x29, 0x02,                  /*         Usage Maximum (02h),         */
        0x25, 0x01,                  /*         Logical Maximum (1),         */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x95, 0x06,                  /*         Report Count (6),            */
        0x81, 0x01,                  /*         Input (Constant),            */
        0x05, 0x01,                  /*         Usage Page (Desktop),        */
        0x09, 0x30,                  /*         Usage (X),                   */
        0x09, 0x31,                  /*         Usage (Y),                   */
        0x15, 0x81,                  /*         Logical Minimum (-127),      */
        0x25, 0x7F,                  /*         Logical Maximum (127),       */
        0x75, 0x08,                  /*         Report Size (8),             */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x81, 0x06,                  /*         Input (Variable, Relative),  */
        0xC0,                        /*     End Collection,                  */
        0xC0,                        /* End Collection,                      */
        0x05, 0x0D,                  /* Usage Page (Digitizer),              */
        0x09, 0x05,                  /* Usage (Touchpad),                    */
        0xA1, 0x01,                  /* Collection (Application),            */
        0x85, 0x04,                  /*     Report ID (4),                   */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x09, 0x22,                  /*     Usage (Finger),                  */
        0xA1, 0x02,                  /*     Collection (Logical),            */
        0x15, 0x00,                  /*         Logical Minimum (0),         */
        0x25, 0x01,                  /*         Logical Maximum (1),         */
        0x09, 0x47,                  /*         Usage (Touch Valid),         */
        0x09, 0x42,                  /*         Usage (Tip Switch),          */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x75, 0x03,                  /*         Report Size (3),             */
        0x25, 0x05,                  /*         Logical Maximum (5),         */
        0x09, 0x51,                  /*         Usage (Contact Identifier),  */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x95, 0x03,                  /*         Report Count (3),            */
        0x81, 0x03,                  /*         Input (Constant, Variable),  */
        0x05, 0x01,                  /*         Usage Page (Desktop),        */
        0x26, 0x44, 0x0A,            /*         Logical Maximum (2628),      */
        0x75, 0x10,                  /*         Report Size (16),            */
        0x55, 0x0E,                  /*         Unit Exponent (14),          */
        0x65, 0x11,                  /*         Unit (Centimeter),           */
        0x09, 0x30,                  /*         Usage (X),                   */
        0x46, 0x1A, 0x04,            /*         Physical Maximum (1050),     */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x46, 0xBC, 0x02,            /*         Physical Maximum (700),      */
        0x26, 0x34, 0x05,            /*         Logical Maximum (1332),      */
        0x09, 0x31,                  /*         Usage (Y),                   */
        0x81, 0x02,                  /*         Input (Variable),            */
        0xC0,                        /*     End Collection,                  */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x09, 0x22,                  /*     Usage (Finger),                  */
        0xA1, 0x02,                  /*     Collection (Logical),            */
        0x25, 0x01,                  /*         Logical Maximum (1),         */
        0x09, 0x47,                  /*         Usage (Touch Valid),         */
        0x09, 0x42,                  /*         Usage (Tip Switch),          */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x75, 0x03,                  /*         Report Size (3),             */
        0x25, 0x05,                  /*         Logical Maximum (5),         */
        0x09, 0x51,                  /*         Usage (Contact Identifier),  */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x95, 0x03,                  /*         Report Count (3),            */
        0x81, 0x03,                  /*         Input (Constant, Variable),  */
        0x05, 0x01,                  /*         Usage Page (Desktop),        */
        0x26, 0x44, 0x0A,            /*         Logical Maximum (2628),      */
        0x75, 0x10,                  /*         Report Size (16),            */
        0x09, 0x30,                  /*         Usage (X),                   */
        0x46, 0x1A, 0x04,            /*         Physical Maximum (1050),     */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x46, 0xBC, 0x02,            /*         Physical Maximum (700),      */
        0x26, 0x34, 0x05,            /*         Logical Maximum (1332),      */
        0x09, 0x31,                  /*         Usage (Y),                   */
        0x81, 0x02,                  /*         Input (Variable),            */
        0xC0,                        /*     End Collection,                  */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x09, 0x22,                  /*     Usage (Finger),                  */
        0xA1, 0x02,                  /*     Collection (Logical),            */
        0x25, 0x01,                  /*         Logical Maximum (1),         */
        0x09, 0x47,                  /*         Usage (Touch Valid),         */
        0x09, 0x42,                  /*         Usage (Tip Switch),          */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x75, 0x03,                  /*         Report Size (3),             */
        0x25, 0x05,                  /*         Logical Maximum (5),         */
        0x09, 0x51,                  /*         Usage (Contact Identifier),  */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x95, 0x03,                  /*         Report Count (3),            */
        0x81, 0x03,                  /*         Input (Constant, Variable),  */
        0x05, 0x01,                  /*         Usage Page (Desktop),        */
        0x26, 0x44, 0x0A,            /*         Logical Maximum (2628),      */
        0x75, 0x10,                  /*         Report Size (16),            */
        0x09, 0x30,                  /*         Usage (X),                   */
        0x46, 0x1A, 0x04,            /*         Physical Maximum (1050),     */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x46, 0xBC, 0x02,            /*         Physical Maximum (700),      */
        0x26, 0x34, 0x05,            /*         Logical Maximum (1332),      */
        0x09, 0x31,                  /*         Usage (Y),                   */
        0x81, 0x02,                  /*         Input (Variable),            */
        0xC0,                        /*     End Collection,                  */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x09, 0x22,                  /*     Usage (Finger),                  */
        0xA1, 0x02,                  /*     Collection (Logical),            */
        0x25, 0x01,                  /*         Logical Maximum (1),         */
        0x09, 0x47,                  /*         Usage (Touch Valid),         */
        0x09, 0x42,                  /*         Usage (Tip Switch),          */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x75, 0x03,                  /*         Report Size (3),             */
        0x25, 0x05,                  /*         Logical Maximum (5),         */
        0x09, 0x51,                  /*         Usage (Contact Identifier),  */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x95, 0x03,                  /*         Report Count (3),            */
        0x81, 0x03,                  /*         Input (Constant, Variable),  */
        0x05, 0x01,                  /*         Usage Page (Desktop),        */
        0x26, 0x44, 0x0A,            /*         Logical Maximum (2628),      */
        0x75, 0x10,                  /*         Report Size (16),            */
        0x09, 0x30,                  /*         Usage (X),                   */
        0x46, 0x1A, 0x04,            /*         Physical Maximum (1050),     */
        0x95, 0x01,                  /*         Report Count (1),            */
        0x81, 0x02,                  /*         Input (Variable),            */
        0x46, 0xBC, 0x02,            /*         Physical Maximum (700),      */
        0x26, 0x34, 0x05,            /*         Logical Maximum (1332),      */
        0x09, 0x31,                  /*         Usage (Y),                   */
        0x81, 0x02,                  /*         Input (Variable),            */
        0xC0,                        /*     End Collection,                  */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x55, 0x0C,                  /*     Unit Exponent (12),              */
        0x66, 0x01, 0x10,            /*     Unit (Seconds),                  */
        0x47, 0xFF, 0xFF, 0x00, 0x00,/*     Physical Maximum (65535),        */
        0x27, 0xFF, 0xFF, 0x00, 0x00,/*     Logical Maximum (65535),         */
        0x75, 0x10,                  /*     Report Size (16),                */
        0x95, 0x01,                  /*     Report Count (1),                */
        0x09, 0x56,                  /*     Usage (Scan Time),               */
        0x81, 0x02,                  /*     Input (Variable),                */
        0x09, 0x54,                  /*     Usage (Contact Count),           */
        0x25, 0x7F,                  /*     Logical Maximum (127),           */
        0x75, 0x08,                  /*     Report Size (8),                 */
        0x81, 0x02,                  /*     Input (Variable),                */
        0x05, 0x09,                  /*     Usage Page (Button),             */
        0x09, 0x01,                  /*     Usage (01h),                     */
        0x25, 0x01,                  /*     Logical Maximum (1),             */
        0x75, 0x01,                  /*     Report Size (1),                 */
        0x95, 0x01,                  /*     Report Count (1),                */
        0x81, 0x02,                  /*     Input (Variable),                */
        0x95, 0x07,                  /*     Report Count (7),                */
        0x81, 0x03,                  /*     Input (Constant, Variable),      */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x85, 0x02,                  /*     Report ID (2),                   */
        0x09, 0x55,                  /*     Usage (Contact Count Maximum),   */
        0x09, 0x59,                  /*     Usage (59h),                     */
        0x75, 0x04,                  /*     Report Size (4),                 */
        0x95, 0x02,                  /*     Report Count (2),                */
        0x25, 0x0F,                  /*     Logical Maximum (15),            */
        0xB1, 0x02,                  /*     Feature (Variable),              */
        0x05, 0x0D,                  /*     Usage Page (Digitizer),          */
        0x85, 0x07,                  /*     Report ID (7),                   */
        0x09, 0x60,                  /*     Usage (60h),                     */
        0x75, 0x01,                  /*     Report Size (1),                 */
        0x95, 0x01,                  /*     Report Count (1),                */
        0x25, 0x01,                  /*     Logical Maximum (1),             */
        0xB1, 0x02,                  /*     Feature (Variable),              */
        0x95, 0x07,                  /*     Report Count (7),                */
        0xB1, 0x03,                  /*     Feature (Constant, Variable),    */
        0x85, 0x06,                  /*     Report ID (6),                   */
        0x06, 0x00, 0xFF,            /*     Usage Page (FF00h),              */
        0x09, 0xC5,                  /*     Usage (C5h),                     */
        0x26, 0xFF, 0x00,            /*     Logical Maximum (255),           */
        0x75, 0x08,                  /*     Report Size (8),                 */
        0x96, 0x00, 0x01,            /*     Report Count (256),              */
        0xB1, 0x02,                  /*     Feature (Variable),              */
        0xC0,                        /* End Collection,                      */
        0x06, 0x00, 0xFF,            /* Usage Page (FF00h),                  */
        0x09, 0x01,                  /* Usage (01h),                         */
        0xA1, 0x01,                  /* Collection (Application),            */
        0x85, 0x0D,                  /*     Report ID (13),                  */
        0x26, 0xFF, 0x00,            /*     Logical Maximum (255),           */
        0x19, 0x01,                  /*     Usage Minimum (01h),             */
        0x29, 0x02,                  /*     Usage Maximum (02h),             */
        0x75, 0x08,                  /*     Report Size (8),                 */
        0x95, 0x02,                  /*     Report Count (2),                */
        0xB1, 0x02,                  /*     Feature (Variable),              */
        0xC0,                        /* End Collection,                      */
        0x05, 0x0D,                  /* Usage Page (Digitizer),              */
        0x09, 0x0E,                  /* Usage (Configuration),               */
        0xA1, 0x01,                  /* Collection (Application),            */
        0x85, 0x03,                  /*     Report ID (3),                   */
        0x09, 0x22,                  /*     Usage (Finger),                  */
        0xA1, 0x02,                  /*     Collection (Logical),            */
        0x09, 0x52,                  /*         Usage (Device Mode),         */
        0x25, 0x0A,                  /*         Logical Maximum (10),        */
        0x95, 0x01,                  /*         Report Count (1),            */
        0xB1, 0x02,                  /*         Feature (Variable),          */
        0xC0,                        /*     End Collection,                  */
        0x09, 0x22,                  /*     Usage (Finger),                  */
        0xA1, 0x00,                  /*     Collection (Physical),           */
        0x85, 0x05,                  /*         Report ID (5),               */
        0x09, 0x57,                  /*         Usage (57h),                 */
        0x09, 0x58,                  /*         Usage (58h),                 */
        0x75, 0x01,                  /*         Report Size (1),             */
        0x95, 0x02,                  /*         Report Count (2),            */
        0x25, 0x01,                  /*         Logical Maximum (1),         */
        0xB1, 0x02,                  /*         Feature (Variable),          */
        0x95, 0x06,                  /*         Report Count (6),            */
        0xB1, 0x03,                  /*         Feature (Constant, Variable),*/
        0xC0,                        /*     End Collection,                  */
        0xC0                         /* End Collection                       */
    };

    memcpy(&hid_descriptor_override, &descriptor_buffer, sizeof(VoodooI2CHIDDeviceHIDDescriptor));

    report_descriptor_override = reinterpret_cast<UInt8*>(IOMalloc(sizeof(report_buffer)));
    memcpy(report_descriptor_override, &report_buffer, sizeof(report_buffer));

    return true;
}

void VoodooI2CHIDSYNA3602Device::free() {
    IOFree(report_descriptor_override, hid_descriptor_override.wReportDescLength);

    super::free();
}
