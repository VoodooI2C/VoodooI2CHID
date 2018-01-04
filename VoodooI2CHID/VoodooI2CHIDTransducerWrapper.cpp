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

VoodooI2CHIDTransducerWrapper* VoodooI2CHIDTransducerWrapper::withElement(IOHIDElement* element, DigitiserTransducerType transducer_type) {
    VoodooI2CHIDTransducerWrapper* wrapper = NULL;
    
    wrapper = new VoodooI2CHIDTransducerWrapper;
    
    if (!wrapper)
        goto exit;
    
    if (!wrapper->init()) {
        wrapper = NULL;
        goto exit;
    }
    
    wrapper->type        = transducer_type;
    wrapper->hid_element     = element;
    
    if (wrapper->hid_element)
        wrapper->hid_element->retain();
    
    wrapper->transducers = OSArray::withCapacity(4);
    
    if (!wrapper->transducers) {
        if (wrapper->hid_element)
            OSSafeReleaseNULL(wrapper->hid_element);
        wrapper = NULL;
        goto exit;
    } else {
        wrapper->hid_element->retain();
    }
    
exit:
    return wrapper;
}
