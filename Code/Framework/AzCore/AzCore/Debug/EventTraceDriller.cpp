/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/EventTraceDriller.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/containers/array.h>
#include <algorithm>

namespace AZ
{
    namespace Debug
    {
        namespace Crc
        {
            const u32 EventTraceDriller = AZ_CRC("EventTraceDriller", 0xf7aeae55);
            const u32 Slice = AZ_CRC("Slice", 0x3dae78a5);
            const u32 ThreadInfo = AZ_CRC("ThreadInfo", 0x89bf78be);
            const u32 Name = AZ_CRC("Name", 0x5e237e06);
            const u32 Category = AZ_CRC("Category", 0x064c19c1);
            const u32 ThreadId = AZ_CRC("ThreadId", 0xd0fd9043);
            const u32 Timestamp = AZ_CRC("Timestamp", 0xa5d6e63e);
            const u32 Duration = AZ_CRC("Duration", 0x865f80c0);
            const u32 Instant = AZ_CRC("Instant", 0x0e9047ad);
        }

        EventTraceDriller::EventTraceDriller()
        {
            EventTraceDrillerSetupBus::Handler::BusConnect();
            AZStd::ThreadDrillerEventBus::Handler::BusConnect();
        }

        EventTraceDriller::~EventTraceDriller()
        {
            AZStd::ThreadDrillerEventBus::Handler::BusDisconnect();
            EventTraceDrillerSetupBus::Handler::BusDisconnect();
        }

        void EventTraceDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            EventTraceDrillerBus::Handler::BusConnect();
            TickBus::Handler::BusConnect();

            EventTraceDrillerBus::AllowFunctionQueuing(true);
        }

        void EventTraceDriller::Stop()
        {
            EventTraceDrillerBus::AllowFunctionQueuing(false);
            EventTraceDrillerBus::ClearQueuedEvents();

            EventTraceDrillerBus::Handler::BusDisconnect();
            TickBus::Handler::BusDisconnect();
        }

        void EventTraceDriller::OnTick(float deltaTime, ScriptTimePoint time)
        {
            (void)deltaTime;
            (void)time;

            AZ_TRACE_METHOD();
            RecordThreads();
            EventTraceDrillerBus::ExecuteQueuedEvents();
        }

        void EventTraceDriller::SetThreadName(const AZStd::thread_id& id, const char* name)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_ThreadMutex);
            m_Threads[(size_t)id.m_id] = ThreadData{ name };
        }

        void EventTraceDriller::OnThreadEnter(const AZStd::thread::id& id, const AZStd::thread_desc* desc)
        {
            if (desc && desc->m_name)
            {
                SetThreadName(id, desc->m_name);
            }
        }

        void EventTraceDriller::OnThreadExit(const AZStd::thread::id& id)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_ThreadMutex);
            m_Threads.erase((size_t)id.m_id);
        }

        void EventTraceDriller::RecordThreads()
        {
            if (m_output && m_Threads.size())
            {
                // Main bus mutex guards m_output.
                auto& context = EventTraceDrillerBus::GetOrCreateContext();

                AZStd::scoped_lock<decltype(context.m_contextMutex), decltype(m_ThreadMutex)> lock(context.m_contextMutex, m_ThreadMutex);
                for (const auto& keyValue : m_Threads)
                {
                    m_output->BeginTag(Crc::EventTraceDriller);
                    m_output->BeginTag(Crc::ThreadInfo);
                    m_output->Write(Crc::ThreadId, keyValue.first);
                    m_output->Write(Crc::Name, keyValue.second.name);
                    m_output->EndTag(Crc::ThreadInfo);
                    m_output->EndTag(Crc::EventTraceDriller);
                }
            }
        }

        void EventTraceDriller::RecordSlice(
            const char* name,
            const char* category,
            const AZStd::thread_id threadId,
            AZ::u64 timestamp,
            AZ::u32 duration)
        {
            m_output->BeginTag(Crc::EventTraceDriller);
            m_output->BeginTag(Crc::Slice);
            m_output->Write(Crc::Name, name);
            m_output->Write(Crc::Category, category);
            m_output->Write(Crc::ThreadId, (size_t)threadId.m_id);
            m_output->Write(Crc::Timestamp, timestamp);
            m_output->Write(Crc::Duration, std::max(duration, 1u));
            m_output->EndTag(Crc::Slice);
            m_output->EndTag(Crc::EventTraceDriller);
        }

        void EventTraceDriller::RecordInstantGlobal(
            const char* name,
            const char* category,
            AZ::u64 timestamp)
        {
            m_output->BeginTag(Crc::EventTraceDriller);
            m_output->BeginTag(Crc::Instant);
            m_output->Write(Crc::Name, name);
            m_output->Write(Crc::Category, category);
            m_output->Write(Crc::Timestamp, timestamp);
            m_output->EndTag(Crc::Instant);
            m_output->EndTag(Crc::EventTraceDriller);
        }

        void EventTraceDriller::RecordInstantThread(
            const char* name,
            const char* category,
            const AZStd::thread_id threadId,
            AZ::u64 timestamp)
        {
            m_output->BeginTag(Crc::EventTraceDriller);
            m_output->BeginTag(Crc::Instant);
            m_output->Write(Crc::Name, name);
            m_output->Write(Crc::Category, category);
            m_output->Write(Crc::ThreadId, (size_t)threadId.m_id);
            m_output->Write(Crc::Timestamp, timestamp);
            m_output->EndTag(Crc::Instant);
            m_output->EndTag(Crc::EventTraceDriller);
        }
    }
}
