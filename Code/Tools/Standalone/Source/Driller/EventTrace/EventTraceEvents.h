/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/Driller/DrillerEvent.h>
#include <AzCore/Debug/EventTraceDrillerBus.h>

namespace Driller
{
    namespace EventTrace
    {
        enum EventType
        {
            ET_SLICE,
            ET_INSTANT,
            ET_THREAD_INFO
        };

        class SliceEvent : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(SliceEvent, AZ::SystemAllocator, 0)

            SliceEvent()
                : DrillerEvent(ET_SLICE)
                , m_Name{ "" }
                , m_Category{ "" }
                , m_ThreadId{}
                , m_Timestamp{}
                , m_Duration{}
            {}

            // no stepping as we can just traverse the list of events
            virtual void StepForward(Aggregator* data) { (void)data; }
            virtual void StepBackward(Aggregator* data) { (void)data; }

            const char* m_Name;
            const char* m_Category;
            size_t m_ThreadId;
            AZStd::sys_time_t m_Timestamp;
            AZStd::sys_time_t m_Duration;
        };

        class InstantEvent : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(InstantEvent, AZ::SystemAllocator, 0)

            InstantEvent()
                : DrillerEvent(ET_INSTANT)
                , m_Name{ "" }
                , m_Category{ "" }
                , m_ThreadId{}
                , m_Timestamp{}
            {}

            // no stepping as we can just traverse the list of events
            virtual void StepForward(Aggregator* data) { (void)data; }
            virtual void StepBackward(Aggregator* data) { (void)data; }

            const char* GetScopeName() const
            {
                return (m_ThreadId == 0) ? "g" : "t";
            }

            const char* m_Name;
            const char* m_Category;
            size_t m_ThreadId;
            AZStd::sys_time_t m_Timestamp;
        };

        class ThreadInfoEvent : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(ThreadInfoEvent, AZ::SystemAllocator, 0)

            ThreadInfoEvent()
                : DrillerEvent(ET_THREAD_INFO)
                , m_ThreadId{}
                , m_Name{ "" }
            {}

            // no stepping as we can just traverse the list of events
            virtual void StepForward(Aggregator* data) { (void)data; }
            virtual void StepBackward(Aggregator* data) { (void)data; }

            size_t m_ThreadId;
            const char* m_Name;
        };
    }

} // namespace Driller
