/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_UNSUPPORTED_EVENTS_H
#define DRILLER_UNSUPPORTED_EVENTS_H

#include "Source/Driller/DrillerEvent.h"

namespace Driller
{
    class UnsupportedEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(UnsupportedEvent, AZ::SystemAllocator, 0)

        UnsupportedEvent(unsigned int eventId)
            : DrillerEvent(eventId) {}

        // no stepping
        virtual void    StepForward(Aggregator* data)   { (void)data; }
        virtual void    StepBackward(Aggregator* data)  { (void)data; }
    };
}

#endif
