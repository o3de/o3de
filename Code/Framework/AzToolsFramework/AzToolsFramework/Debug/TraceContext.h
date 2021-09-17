/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/OSAllocator.h>

#if defined(AZ_ENABLE_TRACING)
#   define AZ_ENABLE_TRACE_CONTEXT
#endif

#ifdef AZ_ENABLE_TRACE_CONTEXT

#include <AzCore/Preprocessor/Sequences.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Math/Uuid.h>

namespace AzToolsFramework
{
    namespace Debug
    {
        // TraceContext provides access to a per-thread stack that can keep track of simple
        //      scoped key-value pairs. Entries are scoped and will automatically be removed once they go 
        //      out of scope. The information gathered through this EBus can be used to provide additional 
        //      information for logging that's collected over multiple function calls. This largely eliminates 
        //      the need to pass context information as parameters to functions or to collect return values to 
        //      reconstruct the context. As this information is available when logging warnings or errors, it 
        //      also helps reduce the amount of log spam.
        //      Several default implementations are provided for handling, recording and formatting the stack
        //      constructed via this EBus, such as the TraceContextStack, TraceContextSingleStackHandler and
        //      the TraceContextLogFormatter.
        //
        //      Usage example:
        //          const char* gameFolder = m_context.pRC->GetSystemEnvironment()->pFileIO->GetAlias("@projectroot@");
        //          AZ_TraceContext("Game folder", gameFolder);
        //
        //          for (int i=0; i<subMeshCount; ++i)
        //          {
        //              AZ_TraceContext("Current mesh", i);
        //              ...
        //
        //          AZ_TraceContext("Scale", 1.5f);
        //
        //          AZ_TraceContext("Merging enabled", true);
        //
        //          AZ::Uuid tag = AZ::Uuid::CreateRandom();
        //          AZ_TraceContext("Mesh Processing Tag", tag);
       
        class TraceContext : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            using AllocatorType = AZ::OSStdAllocator;

            virtual void PushStringEntry(AZStd::thread::id threadId, const char* key, const char* value) = 0;
            virtual void PushBoolEntry(AZStd::thread::id threadId, const char* key, bool value) = 0;
            virtual void PushIntEntry(AZStd::thread::id threadId, const char* key, int64_t value) = 0;
            virtual void PushUintEntry(AZStd::thread::id threadId, const char* key, uint64_t value) = 0;
            virtual void PushFloatEntry(AZStd::thread::id threadId, const char* key, float value) = 0;
            virtual void PushDoubleEntry(AZStd::thread::id threadId, const char* key, double value) = 0;
            virtual void PushUuidEntry(AZStd::thread::id threadId, const char* key, const AZ::Uuid* value) = 0;
            
            virtual void PopEntry(AZStd::thread::id threadId) = 0;
        };
        
        using TraceContextBus = AZ::EBus<TraceContext>;

        class TraceContextScope
        {
        public:
            inline TraceContextScope(const char* key, const char* value);
            inline TraceContextScope(const char* key, const AZStd::string& value);
            inline TraceContextScope(const char* key, const AZ::OSString& value);
            inline TraceContextScope(const char* key, bool value);
            inline TraceContextScope(const char* key, int8_t value);
            inline TraceContextScope(const char* key, int16_t value);
            inline TraceContextScope(const char* key, int32_t value);
            inline TraceContextScope(const char* key, int64_t value);
            inline TraceContextScope(const char* key, uint8_t value);
            inline TraceContextScope(const char* key, uint16_t value);
            inline TraceContextScope(const char* key, uint32_t value);
            inline TraceContextScope(const char* key, uint64_t value);
            inline TraceContextScope(const char* key, float value);
            inline TraceContextScope(const char* key, double value);
            inline TraceContextScope(const char* key, const AZ::Uuid& value);
            inline TraceContextScope(const char* key, const AZ::Uuid* value);

            inline TraceContextScope(const AZStd::string& key, const char* value);
            inline TraceContextScope(const AZStd::string& key, const AZStd::string& value);
            inline TraceContextScope(const AZStd::string& key, const AZ::OSString& value);
            inline TraceContextScope(const AZStd::string& key, bool value);
            inline TraceContextScope(const AZStd::string& key, int8_t value);
            inline TraceContextScope(const AZStd::string& key, int16_t value);
            inline TraceContextScope(const AZStd::string& key, int32_t value);
            inline TraceContextScope(const AZStd::string& key, int64_t value);
            inline TraceContextScope(const AZStd::string& key, uint8_t value);
            inline TraceContextScope(const AZStd::string& key, uint16_t value);
            inline TraceContextScope(const AZStd::string& key, uint32_t value);
            inline TraceContextScope(const AZStd::string& key, uint64_t value);
            inline TraceContextScope(const AZStd::string& key, float value);
            inline TraceContextScope(const AZStd::string& key, double value);
            inline TraceContextScope(const AZStd::string& key, const AZ::Uuid& value);
            inline TraceContextScope(const AZStd::string& key, const AZ::Uuid* value);
            
            inline ~TraceContextScope();
        };
    } // Debug
} // AzToolsFramework

#define AZ_TraceContext(key, value) AzToolsFramework::Debug::TraceContextScope AZ_SEQ_JOIN(traceContextScope, __COUNTER__) (key, value)

#include <AzToolsFramework/Debug/TraceContext.inl>

#else // AZ_ENABLE_TRACE_CONTEXT
        class TraceContext : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            using AllocatorType = AZ::OSStdAllocator;
        };
        
        using TraceContextBus = AZ::EBus<TraceContext>;
#define AZ_TraceContext(key, value) do { (void)key; (void)value; } while(false)

#endif // AZ_ENABLE_TRACE_CONTEXT
