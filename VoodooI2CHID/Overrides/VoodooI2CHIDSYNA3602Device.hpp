//
//  VoodooI2CHIDSYNA3602Device.hpp
//  VoodooI2CHID
//
//  Created by Alexandre Daoud on 10/08/2019.
//Copyright © 2019 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDSYNA3602Device_hpp
#define VoodooI2CHIDSYNA3602Device_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include "VoodooI2CHIDDeviceOverride.hpp"

class VoodooI2CHIDSYNA3602Device : public VoodooI2CHIDDeviceOverride {
  OSDeclareDefaultStructors(VoodooI2CHIDSYNA3602Device);

 public:
    bool init(OSDictionary* properties);
    void free();

 protected:
 private:
};


#endif /* VoodooI2CHIDSYNA3602Device_hpp */
