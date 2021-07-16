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
            EBUS_EVENT(TraceContextBus, PushStringEntry, AZStd::this_thread::get_id(), key , value);
        }

        TraceContextScope::TraceContextScope(const char* key, const AZStd::string& value)
        {
            EBUS_EVENT(TraceContextBus, PushStringEntry, AZStd::this_thread::get_id(), key , value.c_str());
        }

        TraceContextScope::TraceContextScope(const char* key, const AZ::OSString& value)
        {
            EBUS_EVENT(TraceContextBus, PushStringEntry, AZStd::this_thread::get_id(), key, value.c_str());
        }

        TraceContextScope::TraceContextScope(const char* key, bool value)
        {
            EBUS_EVENT(TraceContextBus, PushBoolEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int8_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int16_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int32_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, int64_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint8_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint16_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint32_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, uint64_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, float value)
        {
            EBUS_EVENT(TraceContextBus, PushFloatEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, double value)
        {
            EBUS_EVENT(TraceContextBus, PushDoubleEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const char* key, const AZ::Uuid& value)
        {
            EBUS_EVENT(TraceContextBus, PushUuidEntry, AZStd::this_thread::get_id(), key, &value);
        }

        TraceContextScope::TraceContextScope(const char* key, const AZ::Uuid* value)
        {
            EBUS_EVENT(TraceContextBus, PushUuidEntry, AZStd::this_thread::get_id(), key, value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const char* value)
        {
            EBUS_EVENT(TraceContextBus, PushStringEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZStd::string& value)
        {
            EBUS_EVENT(TraceContextBus, PushStringEntry, AZStd::this_thread::get_id(), key.c_str(), value.c_str());
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZ::OSString& value)
        {
            EBUS_EVENT(TraceContextBus, PushStringEntry, AZStd::this_thread::get_id(), key.c_str(), value.c_str());
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, bool value)
        {
            EBUS_EVENT(TraceContextBus, PushBoolEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int8_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int16_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int32_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, int64_t value)
        {
            EBUS_EVENT(TraceContextBus, PushIntEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint8_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint16_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint32_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, uint64_t value)
        {
            EBUS_EVENT(TraceContextBus, PushUintEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, float value)
        {
            EBUS_EVENT(TraceContextBus, PushFloatEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }
        
        TraceContextScope::TraceContextScope(const AZStd::string& key, double value)
        {
            EBUS_EVENT(TraceContextBus, PushDoubleEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }
        
        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZ::Uuid& value)
        {
            EBUS_EVENT(TraceContextBus, PushUuidEntry, AZStd::this_thread::get_id(), key.c_str(), &value);
        }

        TraceContextScope::TraceContextScope(const AZStd::string& key, const AZ::Uuid* value)
        {
            EBUS_EVENT(TraceContextBus, PushUuidEntry, AZStd::this_thread::get_id(), key.c_str(), value);
        }

        TraceContextScope::~TraceContextScope()
        {
            EBUS_EVENT(TraceContextBus, PopEntry, AZStd::this_thread::get_id());
        }
    } // Debug
} // AzToolsFramework

#endif // AZ_ENABLE_TRACE_CONTEXT
