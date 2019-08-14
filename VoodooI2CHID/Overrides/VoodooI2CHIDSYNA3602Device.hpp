//
//  VoodooI2CHIDSYNA3602Device.hpp
//  VoodooI2CHID
//
//  Created by Alexandre Daoud on 10/08/2019.
//  Copyright © 2019 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDSYNA3602Device_hpp
#define VoodooI2CHIDSYNA3602Device_hpp

#include "VoodooI2CHIDDeviceOverride.hpp"

class VoodooI2CHIDSYNA3602Device : public VoodooI2CHIDDeviceOverride {
  OSDeclareDefaultStructors(VoodooI2CHIDSYNA3602Device);

 public:
    bool init(OSDictionary* properties) override;
    void free() override;
};


#endif /* VoodooI2CHIDSYNA3602Device_hpp */
