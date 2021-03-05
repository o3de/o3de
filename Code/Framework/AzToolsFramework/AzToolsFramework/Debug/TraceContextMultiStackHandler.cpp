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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContextMultiStackHandler.h>

namespace AzToolsFramework
{
    namespace Debug
    {
#ifdef AZ_ENABLE_TRACE_CONTEXT
        TraceContextMultiStackHandler::TraceContextMultiStackHandler()
        {
            BusConnect();
        }
        
        TraceContextMultiStackHandler::~TraceContextMultiStackHandler()
        {
            BusDisconnect();
        }

        void TraceContextMultiStackHandler::PushStringEntry(AZStd::thread::id threadId, const char* key, const char* value)
        {
            GetStackByThreadId(threadId).PushStringEntry(key, value);
        }

        void TraceContextMultiStackHandler::PushBoolEntry(AZStd::thread::id threadId, const char* key, bool value)
        {
            GetStackByThreadId(threadId).PushBoolEntry(key, value);
        }

        void TraceContextMultiStackHandler::PushIntEntry(AZStd::thread::id threadId, const char* key, int64_t value)
        {
            GetStackByThreadId(threadId).PushIntEntry(key, value);
        }

        void TraceContextMultiStackHandler::PushUintEntry(AZStd::thread::id threadId, const char* key, uint64_t value)
        {
            GetStackByThreadId(threadId).PushUintEntry(key, value);
        }

        void TraceContextMultiStackHandler::PushFloatEntry(AZStd::thread::id threadId, const char* key, float value)
        {
            GetStackByThreadId(threadId).PushFloatEntry(key, value);
        }

        void TraceContextMultiStackHandler::PushDoubleEntry(AZStd::thread::id threadId, const char* key, double value)
        {
            GetStackByThreadId(threadId).PushDoubleEntry(key, value);
        }

        void TraceContextMultiStackHandler::PushUuidEntry(AZStd::thread::id threadId, const char* key, const AZ::Uuid* value)
        {
            GetStackByThreadId(threadId).PushUuidEntry(key, value);
        }

        void TraceContextMultiStackHandler::PopEntry(AZStd::thread::id threadId)
        {
            GetStackByThreadId(threadId).PopEntry();
        }

        size_t TraceContextMultiStackHandler::GetStackCount() const
        {
            return m_threadStacks.size();
        }

        uint64_t TraceContextMultiStackHandler::GetStackId(size_t index) const
        {
            if (index < m_threadStacks.size())
            {
                auto iterator = m_threadStacks.begin();
                AZStd::advance(iterator, index);
                return iterator->first;
            }
            else
            {
                return 0;
            }
        }

        AZStd::shared_ptr<const TraceContextStack> TraceContextMultiStackHandler::GetStack(size_t index) const
        {
            if (index < m_threadStacks.size())
            {
                auto iterator = m_threadStacks.begin();
                AZStd::advance(iterator, index);
                return iterator->second;
            }
            else
            {
                return nullptr;
            }
        }

        AZStd::shared_ptr<const TraceContextStack> TraceContextMultiStackHandler::GetCurrentStack() const
        {
            uint64_t currentThreadId = (uint64_t)AZStd::this_thread::get_id().m_id;
            auto entry = m_threadStacks.find(currentThreadId);
            if (entry != m_threadStacks.end())
            {
                return entry->second;
            }
            else
            {
                return nullptr;
            }
        }

        TraceContextStack& TraceContextMultiStackHandler::GetStackByThreadId(AZStd::thread::id threadId)
        {
            // Using a c-style cast because AZStd::hash hasn't been specialized (yet) for thread::id and the type can differ per
            //      platform, requiring static_cast on some and reinterpret_cast on others;
            auto entry = m_threadStacks.find((uint64_t)threadId.m_id);
            if (entry != m_threadStacks.end())
            {
                return *(entry->second);
            }
            else
            {
                AZStd::shared_ptr<TraceContextStack> traceContext = AZStd::make_shared<TraceContextStack>();
                m_threadStacks[(uint64_t)threadId.m_id] = traceContext;
                return *traceContext;
            }
        }

#else // AZ_ENABLE_TRACE_CONTEXT

        TraceContextMultiStackHandler::TraceContextMultiStackHandler()
        {
        }

        TraceContextMultiStackHandler::~TraceContextMultiStackHandler()
        {
        }

        size_t TraceContextMultiStackHandler::GetStackCount() const
        {
            return 0;
        }

        uint64_t TraceContextMultiStackHandler::GetStackId(size_t /*index*/) const
        {
            return 0;
        }

        AZStd::shared_ptr<const TraceContextStack> TraceContextMultiStackHandler::GetStack(size_t /*index*/) const
        {
            return nullptr;
        }

        AZStd::shared_ptr<const TraceContextStack> TraceContextMultiStackHandler::GetCurrentStack() const
        {
            return nullptr;
        }
#endif // AZ_ENABLE_TRACE_CONTEXT
    } // Debug
} // AzToolsFramework
