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

#pragma once

#include <limits>
#include <AzCore/Debug/IEventLogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::Debug
{
    class EventLogReader
    {
    public:
        bool ReadLog(const char* filePath);

        EventNameHash GetEventName() const;
        uint16_t GetEventSize() const;
        uint16_t GetEventFlags() const;
        uint64_t GetThreadId() const;

        AZStd::string_view GetString() const;
        template<typename T>
        const T* GetValue() const
        {
            AZ_Assert(sizeof(T) <= m_current->m_size, "Attempting to retrieve a value that's larger than the amount of stored data.");
            return reinterpret_cast<T*>(m_current + 1);
        }

        bool Next();

    private:
        void UpdateThreadId();

        AZStd::vector<uint8_t> m_buffer;
        IEventLogger::LogHeader m_logHeader;

        uint64_t m_currentThreadId{ 0 };
        IEventLogger::EventHeader* m_current{ nullptr };
    };


    class LocalFileEventLogger
        : public Interface<IEventLogger>::Registrar
    {
    public:
        inline static constexpr size_t MaxThreadCount = 512;

        ~LocalFileEventLogger() override;

        bool Start(const AZ::IO::Path& filePath);
        bool Start(AZStd::string_view outputPath, AZStd::string_view fileNameHint);
        void Stop();

        void Flush() override;

        void* RecordEventBegin(EventNameHash id, uint16_t size, uint16_t flags = 0) override;
        void RecordEventEnd() override;

        void RecordStringEvent(EventNameHash id, AZStd::string_view text, uint16_t flags = 0) override;

    protected:
        struct ThreadData
        {
            // ensure there is enough room for one large event with header + prolog
            static constexpr size_t BufferSize = AZStd::numeric_limits<decltype(EventHeader::m_size)>::max() + sizeof(EventHeader) + sizeof(Prolog);
            char m_buffer[BufferSize]{ 0 };
            uint64_t m_threadId{ 0 };
            uint32_t m_usedBytes{ sizeof(Prolog) }; // always front load the buffer with a prolog
        };

        struct ThreadStorage
        {
            ThreadStorage() = default;
            ~ThreadStorage();

            void Reset(LocalFileEventLogger* owner);

            AZStd::atomic<ThreadData*> m_data{ nullptr };
            ThreadData* m_pendingData{ nullptr };
            LocalFileEventLogger* m_owner{ nullptr };
        };

        void WriteCacheToDisk(ThreadData& threadData);

        ThreadStorage& GetThreadStorage();

        AZStd::fixed_vector<ThreadStorage*, MaxThreadCount> m_threadDataBlocks;

        AZ::IO::SystemFile m_file;
        AZStd::recursive_mutex m_fileGuard;
        AZStd::recursive_mutex m_fileWriteGuard;
    };
} // namespace AZ::Debug
