/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Debug/TraceContextSingleStackHandler.h>

namespace AzToolsFramework
{
    namespace Debug
    {
#ifdef AZ_ENABLE_TRACE_CONTEXT
        TraceContextSingleStackHandler::TraceContextSingleStackHandler()
            : m_targetThread(AZStd::this_thread::get_id())
        {
            BusConnect();
        }

        TraceContextSingleStackHandler::TraceContextSingleStackHandler(AZStd::thread::id targetThread)
            : m_targetThread(targetThread)
        {
            BusConnect();
        }

        TraceContextSingleStackHandler::~TraceContextSingleStackHandler()
        {
            BusDisconnect();
        }

        void TraceContextSingleStackHandler::PushStringEntry(AZStd::thread::id threadId, const char* key, const char* value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushStringEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PushBoolEntry(AZStd::thread::id threadId, const char* key, bool value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushBoolEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PushIntEntry(AZStd::thread::id threadId, const char* key, int64_t value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushIntEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PushUintEntry(AZStd::thread::id threadId, const char* key, uint64_t value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushUintEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PushFloatEntry(AZStd::thread::id threadId, const char* key, float value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushFloatEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PushDoubleEntry(AZStd::thread::id threadId, const char* key, double value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushDoubleEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PushUuidEntry(AZStd::thread::id threadId, const char* key, const AZ::Uuid* value)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PushUuidEntry(key, value);
            }
        }

        void TraceContextSingleStackHandler::PopEntry(AZStd::thread::id threadId)
        {
            if (threadId == m_targetThread)
            {
                m_threadStack.PopEntry();
            }
        }

#else // AZ_ENABLE_TRACE_CONTEXT
        TraceContextSingleStackHandler::TraceContextSingleStackHandler()
        {
        }

        TraceContextSingleStackHandler::TraceContextSingleStackHandler(AZStd::thread::id /*targetThread*/)
        {
        }
        
        TraceContextSingleStackHandler::~TraceContextSingleStackHandler()
        {
        }


#endif // AZ_ENABLE_TRACE_CONTEXT

        AZStd::thread::id TraceContextSingleStackHandler::GetStackThreadId() const
        {
            return m_targetThread;
        }

        const TraceContextStack& TraceContextSingleStackHandler::GetStack() const
        {
            return m_threadStack;
        }
    } // Debug
} // AzToolsFramework
