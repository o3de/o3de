/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/thread.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/Debug/TraceContextStack.h>

namespace AzToolsFramework
{
    namespace Debug
    {
        // TraceContextStack provides a standard implementation for the TraceContextBus that
        //      can be used to track the trace context for a specific thread.
        class TraceContextSingleStackHandler : public TraceContextBus::Handler
        {
        public:
            // Sets up trace context recording for the current thread.
            TraceContextSingleStackHandler();
            explicit TraceContextSingleStackHandler(AZStd::thread::id targetThread);
            ~TraceContextSingleStackHandler() override;

#ifdef AZ_ENABLE_TRACE_CONTEXT
            void PushStringEntry(AZStd::thread::id threadId, const char* key, const char* value) override;
            void PushBoolEntry(AZStd::thread::id threadId, const char* key, bool value) override;
            void PushIntEntry(AZStd::thread::id threadId, const char* key, int64_t value) override;
            void PushUintEntry(AZStd::thread::id threadId, const char* key, uint64_t value) override;
            void PushFloatEntry(AZStd::thread::id threadId, const char* key, float value) override;
            void PushDoubleEntry(AZStd::thread::id threadId, const char* key, double value) override;
            void PushUuidEntry(AZStd::thread::id threadId, const char* key, const AZ::Uuid* value) override;
            
            void PopEntry(AZStd::thread::id threadId) override;
#endif // AZ_ENABLE_TRACE_CONTEXT

            virtual AZStd::thread::id GetStackThreadId() const;
            virtual const TraceContextStack& GetStack() const;

        protected:
            TraceContextStack m_threadStack;
            AZStd::thread::id m_targetThread;
        };
    } // Debug
} // AzToolsFramework
