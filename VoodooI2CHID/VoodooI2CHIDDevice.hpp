//
//  VoodooI2CHIDDevice.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDDevice_hpp
#define VoodooI2CHIDDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

class VoodooI2CDeviceNub;

class VoodooI2CHIDDevice : public IOHIDDevice {
  OSDeclareDefaultStructors(VoodooI2CHIDDevice);

 public:
    bool attach(IOService* provider);
    void detach(IOService* provider);
    bool init(OSDictionary* properties);
    void free();
    VoodooI2CHIDDevice* probe(IOService* provider, SInt32* score);
    bool start(IOService* provider);
    void stop(IOService* provider);

 protected:
 private:
    VoodooI2CDeviceNub* api;
};


#endif /* VoodooI2CHIDDevice_hpp */
