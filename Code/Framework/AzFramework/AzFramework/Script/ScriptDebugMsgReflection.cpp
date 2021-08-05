/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptDebugMsgReflection.h"
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    void ReflectScriptDebugClasses(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AZ::ScriptContextDebug::DebugValue>()
                ->Field("name", &AZ::ScriptContextDebug::DebugValue::m_name)
                ->Field("value", &AZ::ScriptContextDebug::DebugValue::m_value)
                ->Field("type", &AZ::ScriptContextDebug::DebugValue::m_type)
                ->Field("flags", &AZ::ScriptContextDebug::DebugValue::m_flags)
                ->Field("elements", &AZ::ScriptContextDebug::DebugValue::m_elements);

            serializeContext->Class<ScriptUserMethodInfo>()
                ->Field("name", &ScriptUserMethodInfo::m_name)
                ->Field("info", &ScriptUserMethodInfo::m_dbgParamInfo);

            serializeContext->Class<ScriptUserPropertyInfo>()
                ->Field("name", &ScriptUserPropertyInfo::m_name)
                ->Field("isRead", &ScriptUserPropertyInfo::m_isRead)
                ->Field("isWrite", &ScriptUserPropertyInfo::m_isWrite);

            serializeContext->Class<ScriptUserClassInfo>()
                ->Field("name", &ScriptUserClassInfo::m_name)
                ->Field("type", &ScriptUserClassInfo::m_typeId)
                ->Field("methods", &ScriptUserClassInfo::m_methods)
                ->Field("properties", &ScriptUserClassInfo::m_properties);

            serializeContext->Class<ScriptUserEBusMethodInfo, ScriptUserMethodInfo>()
                ->Field("category", &ScriptUserEBusMethodInfo::m_category);

            serializeContext->Class<ScriptUserEBusInfo>()
                ->Field("name", &ScriptUserEBusInfo::m_name)
                ->Field("events", &ScriptUserEBusInfo::m_events)
                ->Field("canBroadcast", &ScriptUserEBusInfo::m_canBroadcast)
                ->Field("canQueue", &ScriptUserEBusInfo::m_canQueue)
                ->Field("hasHandler", &ScriptUserEBusInfo::m_hasHandler);

            serializeContext->Class<ScriptDebugRequest, TmMsg>()
                ->Field("request", &ScriptDebugRequest::m_request)
                ->Field("context", &ScriptDebugRequest::m_context);

            serializeContext->Class<ScriptDebugBreakpointRequest, ScriptDebugRequest>()
                ->Field("line", &ScriptDebugBreakpointRequest::m_line);

            serializeContext->Class<ScriptDebugSetValue, TmMsg>()
                ->Field("value", &ScriptDebugSetValue::m_value);

            serializeContext->Class<ScriptDebugAck, TmMsg>()
                ->Field("request", &ScriptDebugAck::m_request)
                ->Field("ackCode", &ScriptDebugAck::m_ackCode);

            serializeContext->Class<ScriptDebugAckBreakpoint, TmMsg>()
                ->Field("id", &ScriptDebugAckBreakpoint::m_id)
                ->Field("moduleName", &ScriptDebugAckBreakpoint::m_moduleName)
                ->Field("line", &ScriptDebugAckBreakpoint::m_line);

            serializeContext->Class<ScriptDebugAckExecute, TmMsg>()
                ->Field("moduleName", &ScriptDebugAckExecute::m_moduleName)
                ->Field("result", &ScriptDebugAckExecute::m_result);

            serializeContext->Class<ScriptDebugEnumLocalsResult, TmMsg>()
                ->Field("names", &ScriptDebugEnumLocalsResult::m_names);

            serializeContext->Class<ScriptDebugEnumContextsResult, TmMsg>()
                ->Field("names", &ScriptDebugEnumContextsResult::m_names);

            serializeContext->Class<ScriptDebugGetValueResult, TmMsg>()
                ->Field("value", &ScriptDebugGetValueResult::m_value);

            serializeContext->Class<ScriptDebugSetValueResult, TmMsg>()
                ->Field("name", &ScriptDebugSetValueResult::m_name)
                ->Field("result", &ScriptDebugSetValueResult::m_result);

            serializeContext->Class<ScriptDebugCallStackResult, TmMsg>()
                ->Field("callstack", &ScriptDebugCallStackResult::m_callstack);

            serializeContext->Class<ScriptDebugRegisteredGlobalsResult, TmMsg>()
                ->Field("methods", &ScriptDebugRegisteredGlobalsResult::m_methods)
                ->Field("properties", &ScriptDebugRegisteredGlobalsResult::m_properties);

            serializeContext->Class<ScriptDebugRegisteredClassesResult, TmMsg>()
                ->Field("classes", &ScriptDebugRegisteredClassesResult::m_classes);

            serializeContext->Class<ScriptDebugRegisteredEBusesResult, TmMsg>()
                ->Field("EBusses", &ScriptDebugRegisteredEBusesResult::m_ebusList);
        }
    }
}   // namespace AzFramework
