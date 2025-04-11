/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef HEXFRAMEWORK_SCRIPT_DEBUGGER_CLASSES_H
#define HEXFRAMEWORK_SCRIPT_DEBUGGER_CLASSES_H

#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/RTTI.h>

namespace DH {
    struct ClassDataReflection;
}   // namespace DH

namespace AzFramework
{
    class ScriptDebugRequest
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugRequest, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugRequest, "{2137E01A-F2AE-4137-A17E-6B82F3B7E4DE}", RemoteToolsMessage);

        ScriptDebugRequest()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugAgent")) {}
        ScriptDebugRequest(AZ::u32 request)
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugAgent"))
            , m_request(request) {}
        ScriptDebugRequest(AZ::u32 request, const char* context)
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugAgent"))
            , m_request(request)
            , m_context(context) {}

        AZ::u32         m_request;
        AZStd::string   m_context;
    };

    class ScriptDebugBreakpointRequest
        : public ScriptDebugRequest
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugBreakpointRequest, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugBreakpointRequest, "{707F97AB-1CA0-4191-82E0-FFE9C9D0F788}", ScriptDebugRequest);

        ScriptDebugBreakpointRequest() {}
        ScriptDebugBreakpointRequest(AZ::u32 request, const char* context, AZ::u32 line)
            : ScriptDebugRequest(request, context)
            , m_line(line) {}

        AZ::u32 m_line;
    };

    class ScriptDebugSetValue
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugSetValue, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugSetValue, "{11E0E012-BD54-457D-A44B-FDDA55736ED3}", RemoteToolsMessage);

        ScriptDebugSetValue()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugAgent")) {}

        AZ::ScriptContextDebug::DebugValue  m_value;
    };

    class ScriptDebugAck
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugAck, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugAck, "{0CA1671A-BAFD-499C-B2CD-7B7E3DD5E2A8}", RemoteToolsMessage);

        ScriptDebugAck(AZ::u32 request = 0, AZ::u32 ackCode = 0)
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger"))
            , m_request(request)
            , m_ackCode(ackCode)
        {}

        AZ::u32 m_request;
        AZ::u32 m_ackCode;
    };

    class ScriptDebugAckBreakpoint
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugAckBreakpoint, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugAckBreakpoint, "{D9644B8A-92FD-43B6-A579-77E123A72EC2}",  RemoteToolsMessage);

        ScriptDebugAckBreakpoint()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZ::u32         m_id;
        AZStd::string   m_moduleName;
        AZ::u32         m_line;
    };

    class ScriptDebugAckExecute
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugAckExecute, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugAckExecute, "{F5B24F7E-85DA-4FE8-B720-AABE35CE631D}", RemoteToolsMessage);

        ScriptDebugAckExecute()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZStd::string   m_moduleName;
        bool            m_result;
    };

    class ScriptDebugEnumLocalsResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugEnumLocalsResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugEnumLocalsResult, "{201701DD-0B74-4886-AB84-93BDB338A8DD}", RemoteToolsMessage);

        ScriptDebugEnumLocalsResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZStd::vector<AZStd::string>    m_names;
    };

    class ScriptDebugEnumContextsResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugEnumContextsResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugEnumContextsResult, "{8CE74569-9B7D-4993-AFE8-38BB8CE419F5}", RemoteToolsMessage);

        ScriptDebugEnumContextsResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZStd::vector<AZStd::string>    m_names;
    };

    class ScriptDebugGetValueResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugGetValueResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugGetValueResult, "{B10720F1-B8FE-476F-A39D-6E80711580FD}", RemoteToolsMessage);

        ScriptDebugGetValueResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZ::ScriptContextDebug::DebugValue  m_value;
    };

    class ScriptDebugSetValueResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugSetValueResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugSetValueResult, "{2E2BD168-1805-43D6-8602-FDE14CED8E53}", RemoteToolsMessage);

        ScriptDebugSetValueResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZStd::string   m_name;
        bool            m_result;
    };

    class ScriptDebugCallStackResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugCallStackResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugCallStackResult, "{B2606AC6-F966-4991-8144-BA6117F4A54E}", RemoteToolsMessage);

        ScriptDebugCallStackResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        AZStd::string   m_callstack;
    };

    class ScriptDebugRegisteredGlobalsResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugRegisteredGlobalsResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugRegisteredGlobalsResult, "{CEE4E889-0249-4D59-9D56-CD4BD159E411}", RemoteToolsMessage);

        ScriptDebugRegisteredGlobalsResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        ScriptUserMethodList    m_methods;
        ScriptUserPropertyList  m_properties;
    };

    class ScriptDebugRegisteredClassesResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugRegisteredClassesResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugRegisteredClassesResult, "{7DF455AB-9AB1-4A95-B906-5DB1D1087EBB}", RemoteToolsMessage);

        ScriptDebugRegisteredClassesResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        ScriptUserClassList m_classes;
    };

    class ScriptDebugRegisteredEBusesResult
        : public RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptDebugRegisteredEBusesResult, AZ::SystemAllocator);
        AZ_RTTI(ScriptDebugRegisteredEBusesResult, "{D2B5D77C-09F3-476D-A611-49B0A1B9EDFB}", RemoteToolsMessage);

        ScriptDebugRegisteredEBusesResult()
            : RemoteToolsMessage(AZ_CRC_CE("ScriptDebugger")) {}

        ScriptUserEBusList m_ebusList;
    };

    void ReflectScriptDebugClasses(AZ::ReflectContext* reflection);
}   // namespace AzFramework

#endif  // HEXFRAMEWORK_SCRIPT_DEBUGGER_CLASSES_H
#pragma once
