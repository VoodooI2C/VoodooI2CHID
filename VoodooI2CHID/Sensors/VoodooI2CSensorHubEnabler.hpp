//
//  VoodooI2CSensorHubEnabler.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CSensorHubEnabler_hpp
#define VoodooI2CSensorHubEnabler_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "../../../../Dependencies/helpers.hpp"

class EXPORT VoodooI2CSensorHubEnabler : public IOService {
  OSDeclareDefaultStructors(VoodooI2CSensorHubEnabler);

 public:
    bool start(IOService* provider) override;
 protected:
 private:
    const char* name;
    IOACPIPlatformDevice* provider;
    
    IOReturn enableHub();
};


#endif /* VoodooI2CSensorHubEnabler_hpp */
