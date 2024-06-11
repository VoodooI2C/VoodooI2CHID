//
//  VoodooI2CHIDTrackpointWrapper.cpp
//  VoodooI2CHID
//
//  Created by Avery Black on 5/31/24.
//  Copyright © 2024 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CHIDTrackpointWrapper.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(VoodooI2CHIDTrackpointWrapper, OSObject);

VoodooI2CHIDTrackpointWrapper* wrapper() {
    auto *tp = OSTypeAlloc(VoodooI2CHIDTrackpointWrapper);
    if (tp == nullptr) {
        return nullptr;
    }
    
    if (!tp->init()) {
        OSSafeReleaseNULL(tp);
        return nullptr;
    }
    
    return tp;
}

bool VoodooI2CHIDTrackpointWrapper::init() {
    if (!super::init()) {
        return false;
    }
    
    buttons = OSArray::withCapacity(1);
    return buttons != nullptr;
}

void VoodooI2CHIDTrackpointWrapper::free() {
    OSSafeReleaseNULL(buttons);
    super::free();
}
