/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef AZ_ENABLE_TRACE_CONTEXT

namespace AzToolsFramework
{
    namespace Debug
    {
        TraceContextScope::TraceContextScope(const char* key, const char* value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushStringEntry, AZStd::this_thread::get_id(), key , value);
        }

        TraceContextScope::TraceContextScope(const char* key, const AZStd::string& value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushStringEntry, AZStd::this_thread::get_id(), key , value.c_str());
        }

        TraceContextScope::TraceContextScope(const char* key, const AZ::OSString& value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushStringEntry, AZStd::this_thread::get_id(), key, value.c_str());
        }

        TraceContextScope::TraceContextScope(const char* key, bool value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushBoolEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int8_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int16_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int32_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int64_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint8_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint16_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint32_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint64_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, float value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushFloatEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, double value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushDoubleEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, const AZ::Uuid& value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUuidEntry, AZStd::this_thread::get_id(), key, &value);
        }

        TraceContextScope::TraceContextScope(const char* key, const AZ::Uuid* value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUuidEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const char* value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushStringEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZStd::string& value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushStringEntry, AZStd::this_thread::get_id(), key.c_str(), value.c_str());
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZ::OSString& value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushStringEntry, AZStd::this_thread::get_id(), key.c_str(), value.c_str());
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, bool value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushBoolEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int8_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int16_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int32_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int64_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint8_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint16_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint32_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint64_t value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, float value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushFloatEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }
        
        TraceContextScope::TraceContextScope(const AZStd::string& key, double value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushDoubleEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }
        
        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZ::Uuid& value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUuidEntry, AZStd::this_thread::get_id(), key.c_str(), &value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZ::Uuid* value)
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PushUuidEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::~TraceContextScope()
        {
            TraceContextBus::Broadcast(&TraceContextBus::Events::PopEntry, AZStd::this_thread::get_id());
        }
    } // Debug
} // AzToolsFramework

#endif // AZ_ENABLE_TRACE_CONTEXT
