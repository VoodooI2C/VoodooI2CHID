//
//  VoodooI2CHIDDeviceOverride.hpp
//  VoodooI2CHID
//
//  Created by Alexandre Daoud on 10/08/2019.
//  Copyright © 2019 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDDeviceOverride_hpp
#define VoodooI2CHIDDeviceOverride_hpp

#include "../VoodooI2CHIDDevice.hpp"

class VoodooI2CHIDDeviceOverride : public VoodooI2CHIDDevice {
  OSDeclareDefaultStructors(VoodooI2CHIDDeviceOverride);

 protected:
    VoodooI2CHIDDeviceHIDDescriptor hid_descriptor_override;
    UInt8* report_descriptor_override;
    
    IOReturn getHIDDescriptor() override;
    IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
};


#endif /* VoodooI2CHIDDeviceOverride_hpp */
