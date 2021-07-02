/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Driller/DrillerBus.h>
#include <AzCore/std/time.h>
#include <AzCore/std/string/string.h>

namespace AZStd
{
    struct thread_id;
}

namespace AZ
{
    namespace Debug
    {
        class EventTraceDrillerInterface
            : public DrillerEBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const bool EnableEventQueue = true;
            static const bool EventQueueingActiveByDefault = false;
            //////////////////////////////////////////////////////////////////////////

            virtual ~EventTraceDrillerInterface() {}

            virtual void RecordSlice(
                const char* name,
                const char* category,
                const AZStd::thread_id threadId,
                AZ::u64 timestamp,
                AZ::u32 duration) = 0;

            virtual void RecordInstantThread(
                const char* name,
                const char* category,
                const AZStd::thread_id threadId,
                AZ::u64 timestamp) = 0;

            virtual void RecordInstantGlobal(
                const char* name,
                const char* category,
                AZ::u64 timestamp) = 0;
        };

        typedef AZ::EBus<EventTraceDrillerInterface> EventTraceDrillerBus;

        class EventTraceDrillerSetupInterface
            : public DrillerEBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            virtual ~EventTraceDrillerSetupInterface() {}

            virtual void SetThreadName(const AZStd::thread_id& threadId, const char* name) = 0;
        };

        typedef AZ::EBus<EventTraceDrillerSetupInterface> EventTraceDrillerSetupBus;
    }
}

#define AZ_TRACE_INSTANT_GLOBAL_CATEGORY(name, category) \
    EBUS_QUEUE_EVENT(AZ::Debug::EventTraceDrillerBus, RecordInstantGlobal, name, category, AZStd::GetTimeNowMicroSecond())
#define AZ_TRACE_INSTANT_GLOBAL(name) AZ_TRACE_INSTANT_GLOBAL_CATEGORY(name, "")

#define AZ_TRACE_INSTANT_THREAD_CATEGORY(name, category) \
    EBUS_QUEUE_EVENT(AZ::Debug::EventTraceDrillerBus, RecordInstantThread, name, category, AZStd::this_thread::get_id(), AZStd::GetTimeNowMicroSecond())
#define AZ_TRACE_INSTANT_THREAD(name) AZ_TRACE_INSTANT_THREAD_CATEGORY(name, "")
