/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Source/Driller/DrillerEvent.h"

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
