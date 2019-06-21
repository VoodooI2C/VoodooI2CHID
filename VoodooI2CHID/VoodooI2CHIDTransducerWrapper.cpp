//
//  VoodooI2CHIDTransducerWrapper.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 02/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CHIDTransducerWrapper.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(VoodooI2CHIDTransducerWrapper, OSObject);

void VoodooI2CHIDTransducerWrapper::free() {
    OSSafeReleaseNULL(transducers);

    super::free();
}

VoodooI2CHIDTransducerWrapper* VoodooI2CHIDTransducerWrapper::wrapper() {
    VoodooI2CHIDTransducerWrapper* wrapper = NULL;
    
    wrapper = OSTypeAlloc(VoodooI2CHIDTransducerWrapper);
    
    if (!wrapper)
        goto exit;
    
    if (!wrapper->init()) {
        OSSafeReleaseNULL(wrapper);
        goto exit;
    }
    wrapper->transducers = OSArray::withCapacity(4);
    
exit:
    return wrapper;
}
