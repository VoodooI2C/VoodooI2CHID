//
//  VoodooI2CHIDTrackpointWrapper.hpp
//  VoodooI2CHID
//
//  Created by Avery Black on 5/31/24.
//  Copyright © 2024 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDTrackpointWrapper_hpp
#define VoodooI2CHIDTrackpointWrapper_hpp

#include <IOKit/hid/IOHIDElement.h>

class VoodooI2CHIDTrackpointWrapper : public OSObject {
    OSDeclareDefaultStructors(VoodooI2CHIDTrackpointWrapper);
public:
    static VoodooI2CHIDTrackpointWrapper* wrapper();
    
    bool init() override;
    void free() override;
    
    UInt8           report_id = 0;
    IOHIDElement*   dx = nullptr;
    IOHIDElement*   dy = nullptr;
    OSArray*        buttons = nullptr;
};

#endif /* VoodooI2CHIDTrackpointWrapper_hpp */
