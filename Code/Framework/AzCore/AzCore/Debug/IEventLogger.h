/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <type_traits>
#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::Debug
{
    //! Simple hash structure based on DJB2a to generate event IDs at compile time
    class EventNameHash
    {
    public:
        constexpr explicit EventNameHash(AZStd::string_view name)
        {
            for (auto c : name)
            {
                m_hash = static_cast<uint32_t>(((static_cast<uint64_t>(m_hash) << 5) + m_hash) ^ c);
            }
        }

        constexpr bool operator==(const EventNameHash& rhs) const
        {
            return m_hash == rhs.m_hash;
        }

        constexpr bool operator!=(const EventNameHash& rhs) const
        {
            return m_hash != rhs.m_hash;
        }

    private:
        uint32_t m_hash{ 5381 }; // standard starting value for DJB2a hash
    };


    constexpr EventNameHash PrologEventHash("Prolog");
    constexpr uint16_t EventBoundary = 8;


    class IEventLogger
    {
    public:
        struct LogHeader
        {
            int8_t m_4cc[4]{ 'A', 'Z', 'E', 'L' };  //!< 4CC to uniquely identify the data type. Defaults to 'AZEL'
            uint32_t m_majorVersion{ 1 };           //!< Major version of the log format.
            uint32_t m_minorVersion{ 0 };           //!< Minor version of the log format.
            //! A user defined version. This will always be zero but allows users
            //! make modifications without needing to change the main version number
            //! which in turn makes integrations easier.
            uint32_t m_userVersion{ 0 };
        };

        struct EventHeader
        {
            EventNameHash m_eventId;    //!< Unique id that identifies the event. This is typically a hash of the event name.
            uint16_t m_size;            //!< The size of the event. Events can be up to 64Kib large.
            //! Event specific flags set by the caller. The flags can be to reuse the same event with a slight
            //! alteration, such as a begin/end pair. If two similar have different data, such as a begin
            //! having a bit of extra data that the end doesn't have, then it's recommended to create two unique
            //! events instead to keep the log small.
            uint16_t m_flags;
        };

        struct Prolog : public EventHeader
        {
            uint64_t m_threadId;    //!< Unique id of the thread the log buffer is being recorded on.
        };

        AZ_TYPE_INFO(AZ::Debug::IEventLogger, "{D39D09FA-DEA0-4874-BC45-4B310C3DD52E}");

        virtual ~IEventLogger() = default;

        //! Writes and flushes all thread local buffers to disk and flushes the disk to store the recorded events.
        virtual void Flush() = 0;

        //! Starts a new event. If there is not enough room left in the thread local buffer then the buffer will be stored to disk and cleared.
        //! @param id Id that uniquely identifies this event.
        //! @param size The total size of the event, excluding the event header. Typically this is the size of the structure that describes the event.
        //! @param flags Optional flags unique to the event. For instance a "Thread" event can use the flags to indicate whether the
        //!      the thread is starting or stopping.
        //! @return A void pointer to reserved data in the thread local buffer to write to.
        virtual void* RecordEventBegin(EventNameHash id, uint16_t size, uint16_t flags = 0) = 0;

        //! End a previously started event. After calling RecordEventBegin, flushing will not be possible until RecordEventEnd is called.
        virtual void RecordEventEnd() = 0;

        //! Utility function to write an event that only has a string.
        //! @param id Id that uniquely identifies this event.
        //! @param text The string that will be logged.
        //! @param flags Optional flags unique to the event.
        virtual void RecordStringEvent(EventNameHash id, AZStd::string_view text, uint16_t flags = 0) = 0;

        //! Utility function to begin an event with a specific structure.
        //! For example this can be used as:
        //!      struct ThreadInfo
        //!      {
        //!          uint64_t m_threadId;
        //!          uint64_t m_processorId;
        //!      };
        //!      auto& info = RecordEventBegin<ThreadInfo>("ThreadInfo");
        //!      info.m_threadId = ...;
        //!      info.m_processorId = ...;
        //!      RecordEventEnd();
        //! @param id Id that uniquely identifies this event.
        //! @param flags Optional flags unique to the event.
        //! @return A reference of the provided type to store event information in.
        template<typename T>
        T& RecordEventBegin(EventNameHash id, uint16_t flags = 0);
    };

    template<typename T>
    T& IEventLogger::RecordEventBegin(EventNameHash id, uint16_t flags)
    {
        constexpr size_t typeSize = sizeof(T);

        static_assert(AZStd::is_trivially_copyable_v<T>, "Only simple classes can be added to the event logger.");
        static_assert(typeSize <= AZStd::numeric_limits<decltype(EventHeader::m_size)>::max(), "Class too large to store with the event logger.");

        void* eventData = RecordEventBegin(id, aznumeric_cast<uint16_t>(typeSize), flags);
        return *reinterpret_cast<T*>(eventData);
    }
} // namespace AZ::Debug
