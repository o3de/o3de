/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/Debug/TraceContextStack.h>

namespace AzToolsFramework
{
    namespace Debug
    {
        // TraceContextMultiStack provides a standard implementation for the TraceContextBus that 
        //      can be used for tracking trace context stacks for multiple threads.
        class TraceContextMultiStackHandler : public TraceContextBus::Handler
        {
        public:
            TraceContextMultiStackHandler();
            ~TraceContextMultiStackHandler() override;

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

            virtual size_t GetStackCount() const;
            virtual uint64_t GetStackId(size_t index) const;
            virtual AZStd::shared_ptr<const TraceContextStack> GetStack(size_t index) const;
            // Returns the stack for the current stack if available, otherwise null.
            virtual AZStd::shared_ptr<const TraceContextStack> GetCurrentStack() const;

        protected:
#ifdef AZ_ENABLE_TRACE_CONTEXT
            virtual TraceContextStack& GetStackByThreadId(AZStd::thread::id threadId);
#endif

            AZStd::unordered_map<uint64_t, AZStd::shared_ptr<TraceContextStack>> m_threadStacks;
        };
    } // Debug
} // AzToolsFramework
