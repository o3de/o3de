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

#ifndef DRILLER_TRACE_MESSAGE_EVENTS_H
#define DRILLER_TRACE_MESSAGE_EVENTS_H

#include "Woodpecker/Driller/DrillerEvent.h"

#include <AzCore/Debug/TraceMessagesDrillerBus.h>

namespace Driller
{
    class TraceMessageEvent
        : public DrillerEvent
    {
    public:
        enum EventType
        {
            ET_ASSERT,
            ET_EXCEPTION,
            ET_ERROR,
            ET_WARNING,
            ET_PRINTF,
        };

        AZ_CLASS_ALLOCATOR(TraceMessageEvent, AZ::SystemAllocator, 0)

        TraceMessageEvent(EventType eventType)
            : DrillerEvent(eventType)
            , m_window(nullptr)
            , m_message(nullptr)
            , m_windowCRC(0)
        {}

        void ComputeCRC()
        {
            if (m_window)
            {
                m_windowCRC = AZ::Crc32(m_window);
            }
            else
            {
                m_windowCRC = 0;
            }
        }

        // no stepping as we can just traverse the list of events
        virtual void    StepForward(Aggregator* data)       { (void)data; }
        virtual void    StepBackward(Aggregator* data)      { (void)data; }

        const char*     m_window;       ///< Name of the message window
        const char*     m_message;
        AZ::u32         m_windowCRC; // so that we dont constantly take a penalty of CRCing for every event when we do the annotations
    };
}

#endif //Woodpecker_TRACE_MESSAGE_EVENTS_H
