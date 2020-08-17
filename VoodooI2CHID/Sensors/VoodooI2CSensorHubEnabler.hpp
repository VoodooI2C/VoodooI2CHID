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

#define SHAD_DSM_GUID "03c868d5-563f-42a8-9f57-9a18d949b7cb"
#define SHAD_DSM_REVISION 1
#define SHAD_ON_INDEX 1

class EXPORT VoodooI2CSensorHubEnabler : public IOService {
  OSDeclareDefaultStructors(VoodooI2CSensorHubEnabler);

 public:
    bool start(IOService* provider);
 protected:
 private:
    const char* name;
    IOACPIPlatformDevice* provider;
    
    IOReturn enableHub(UInt32 *result);
};


#endif /* VoodooI2CSensorHubEnabler_hpp */
