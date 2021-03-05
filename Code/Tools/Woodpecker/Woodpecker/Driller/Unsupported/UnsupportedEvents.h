/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef DRILLER_UNSUPPORTED_EVENTS_H
#define DRILLER_UNSUPPORTED_EVENTS_H

#include "Woodpecker/Driller/DrillerEvent.h"

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
