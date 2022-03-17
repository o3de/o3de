/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUADebuggerComponent.h"
#include "LUAEditorContextMessages.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Script/ScriptContext.h>

#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzFramework/Script/ScriptDebugMsgReflection.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>


namespace LUADebugger
{
    // Utility functions
    // Returns true if a valid target was found, in which case the info is returned in targetInfo.
    bool GetDesiredTarget(AzFramework::TargetInfo& targetInfo)
    {
        // discover what target the user is currently connected to, if any?
        AzFramework::TargetInfo info;
        EBUS_EVENT_RESULT(info, AzFramework::TargetManager::Bus, GetDesiredTarget);
        if (!info.GetPersistentId())
        {
            AZ_TracePrintf("Debug", "The user has not chosen a target to connect to.\n");
            return false;
        }

        targetInfo = info;
        if (
            (!targetInfo.IsValid()) ||
            ((targetInfo.GetStatusFlags() & AzFramework::TF_ONLINE) == 0) ||
            ((targetInfo.GetStatusFlags() & AzFramework::TF_DEBUGGABLE) == 0)
            )
        {
            AZ_TracePrintf("Debug", "The target is currently not in a state that would allow debugging code (offline or not debuggable)\n");
            return false;
        }
        return true;
    }

    Component::Component()
    {
    }

    Component::~Component()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Component::Init()
    {
    }

    void Component::Activate()
    {
        LUAEditorDebuggerMessages::Bus::Handler::BusConnect();
        AzFramework::TargetManagerClient::Bus::Handler::BusConnect();
        AzFramework::TmMsgBus::Handler::BusConnect(AZ_CRC("ScriptDebugger", 0xf8ab685e));
    }

    void Component::Deactivate()
    {
        LUAEditorDebuggerMessages::Bus::Handler::BusDisconnect();
        AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();
        AzFramework::TmMsgBus::Handler::BusDisconnect(AZ_CRC("ScriptDebugger", 0xf8ab685e));
    }

    void Component::EnumerateContexts()
    {
        AZ_TracePrintf("LUA Debug", "Component::EnumerateContexts()\n");

        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("EnumContexts", 0xbdb959ba)));
        }
    }

    void Component::AttachDebugger(const char* scriptContextName)
    {
        AZ_TracePrintf("LUA Debug", "Component::AttachDebugger( %s )\n", scriptContextName);

        AZ_Assert(scriptContextName, "You need to supply a valid script context name to attach to!");
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("AttachDebugger", 0x6590ff36), scriptContextName));
        }
    }

    void Component::DetachDebugger()
    {
        AZ_TracePrintf("LUA Debug", "Component::DetachDebugger()\n");

        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("DetachDebugger", 0x88a2ee04)));
        }
    }

    void Component::EnumRegisteredClasses(const char* scriptContextName)
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("EnumRegisteredClasses", 0xed6b8070), scriptContextName));
        }
    }

    void Component::EnumRegisteredEBuses(const char* scriptContextName)
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("EnumRegisteredEBuses"), scriptContextName));
        }
    }

    void Component::EnumRegisteredGlobals(const char* scriptContextName)
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("EnumRegisteredGlobals", 0x80d1e6af), scriptContextName));
        }
    }

    void Component::CreateBreakpoint(const AZStd::string& debugName, int lineNumber)
    {
        // register a breakpoint.

        // Debug name will be the full, absolute path, so convert it to the relative path
        AZStd::string relativePath = debugName;
        EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, GetRelativeProductPathFromFullSourceOrProductPath, debugName, relativePath);
        relativePath = "@" + relativePath;

        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            // local editors are never debuggable (they'd never have the debuggable flag) so if you get here you know its over the network
            // and its network id is targetID.
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugBreakpointRequest(AZ_CRC("AddBreakpoint", 0xba71daa4), relativePath.c_str(), static_cast<AZ::u32>(lineNumber)));
        }
    }

    void Component::RemoveBreakpoint(const AZStd::string& debugName, int lineNumber)
    {
        // remove a breakpoint.

        // Debug name will be the full, absolute path, so convert it to the relative path
        AZStd::string relativePath = debugName;
        EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, GetRelativeProductPathFromFullSourceOrProductPath, debugName, relativePath);
        relativePath = "@" + relativePath;

        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            // local editors are never debuggable (they'd never have the debuggable flag) so if you get here you know its over the network
            // and its network id is targetID.
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugBreakpointRequest(AZ_CRC("RemoveBreakpoint", 0x90ade500), relativePath.c_str(), static_cast<AZ::u32>(lineNumber)));
        }
    }

    void Component::DebugRunStepOver()
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("StepOver", 0x6b89bf41)));
        }
    }

    void Component::DebugRunStepIn()
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("StepIn", 0x761a6b13)));
        }
    }

    void Component::DebugRunStepOut()
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("StepOut", 0xac19b635)));
        }
    }

    void Component::DebugRunStop()
    {
        // Script context can't be stopped. What should we do here?
    }

    void Component::DebugRunContinue()
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("Continue", 0x13e32adf)));
        }
    }

    void Component::EnumLocals()
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("EnumLocals", 0x4aa29dcf)));
        }
    }

    void Component::GetValue(const AZStd::string& varName)
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("GetValue", 0x2d64f577), varName.c_str()));
        }
    }

    void Component::SetValue(const AZ::ScriptContextDebug::DebugValue& value)
    {
        (void)value;
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            AzFramework::ScriptDebugSetValue request;
            request.m_value = value;
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, request);
        }
    }

    void Component::GetCallstack()
    {
        AzFramework::TargetInfo targetInfo;
        if (GetDesiredTarget(targetInfo))
        {
            EBUS_EVENT(AzFramework::TargetManager::Bus, SendTmMessage, targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC("GetCallstack", 0x343b24f3)));
        }
    }

    void Component::OnReceivedMsg(AzFramework::TmMsgPtr msg)
    {
        if (AzFramework::ScriptDebugAck* ack = azdynamic_cast<AzFramework::ScriptDebugAck*>(msg.get()))
        {
            if (ack->m_ackCode == AZ_CRC("Ack", 0x22e4f8b1))
            {
                if (ack->m_request == AZ_CRC("Continue", 0x13e32adf) || ack->m_request == AZ_CRC("StepIn", 0x761a6b13) || ack->m_request == AZ_CRC("StepOut", 0xac19b635) || ack->m_request == AZ_CRC("StepOver", 0x6b89bf41))
                {
                    EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnExecutionResumed);
                }
                else if (ack->m_request == AZ_CRC("AttachDebugger", 0x6590ff36))
                {
                    EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnDebuggerAttached);
                }
                else if (ack->m_request == AZ_CRC("DetachDebugger", 0x88a2ee04))
                {
                    EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnDebuggerDetached);
                }
            }
            else if (ack->m_ackCode == AZ_CRC("IllegalOperation", 0x437dc900))
            {
                if (ack->m_request == AZ_CRC("ExecuteScript", 0xc35e01e7))
                {
                    EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnExecuteScriptResult, false);
                }
                else if (ack->m_request == AZ_CRC("AttachDebugger", 0x6590ff36))
                {
                    EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnDebuggerRefused);
                }
                else
                {
                    AZ_TracePrintf("LUA Debug", "Debug Agent: Illegal operation 0x%x. Script context is in the wrong state.\n", ack->m_request);
                }
            }
            else if (ack->m_ackCode == AZ_CRC("AccessDenied", 0xde72ce21))
            {
                AZ_TracePrintf("LUA Debug", "Debug Agent: Access denied 0x%x. Attach debugger first!\n", ack->m_request);
                EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnDebuggerDetached);
            }
            else if (ack->m_ackCode == AZ_CRC("InvalidCmd", 0x926abd27))
            {
                AZ_TracePrintf("LUA Debug", "The remote script debug agent claims that we sent it an invalid request(0x%x)!\n", ack->m_request);
            }
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugAckBreakpoint*>(msg.get()))
        {
            AzFramework::ScriptDebugAckBreakpoint* ackBreakpoint = azdynamic_cast<AzFramework::ScriptDebugAckBreakpoint*>(msg.get());
            if (ackBreakpoint->m_id == AZ_CRC("BreakpointHit", 0xf1a38e0b))
            {
                EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnBreakpointHit, ackBreakpoint->m_moduleName, ackBreakpoint->m_line);
            }
            else if (ackBreakpoint->m_id == AZ_CRC("AddBreakpoint", 0xba71daa4))
            {
                EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnBreakpointAdded, ackBreakpoint->m_moduleName, ackBreakpoint->m_line);
            }
            else if (ackBreakpoint->m_id == AZ_CRC("RemoveBreakpoint", 0x90ade500))
            {
                EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnBreakpointRemoved, ackBreakpoint->m_moduleName, ackBreakpoint->m_line);
            }
        }
        else if (AzFramework::ScriptDebugAckExecute* ackExecute = azdynamic_cast<AzFramework::ScriptDebugAckExecute*>(msg.get()))
        {
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnExecuteScriptResult, ackExecute->m_result);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugEnumLocalsResult*>(msg.get()))
        {
            AzFramework::ScriptDebugEnumLocalsResult* enumLocals = azdynamic_cast<AzFramework::ScriptDebugEnumLocalsResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedLocalVariables, enumLocals->m_names);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugEnumContextsResult*>(msg.get()))
        {
            AzFramework::ScriptDebugEnumContextsResult* enumContexts = azdynamic_cast<AzFramework::ScriptDebugEnumContextsResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedAvailableContexts, enumContexts->m_names);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugGetValueResult*>(msg.get()))
        {
            AzFramework::ScriptDebugGetValueResult* getValues = azdynamic_cast<AzFramework::ScriptDebugGetValueResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedValueState, getValues->m_value);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugSetValueResult*>(msg.get()))
        {
            AzFramework::ScriptDebugSetValueResult* setValue = azdynamic_cast<AzFramework::ScriptDebugSetValueResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnSetValueResult, setValue->m_name, setValue->m_result);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugCallStackResult*>(msg.get()))
        {
            AzFramework::ScriptDebugCallStackResult* callStackResult = azdynamic_cast<AzFramework::ScriptDebugCallStackResult*>(msg.get());
            AZStd::vector<AZStd::string> callstack;
            const char* c1 = callStackResult->m_callstack.c_str();
            for (const char* c2 = c1; *c2; ++c2)
            {
                if (*c2 == '\n')
                {
                    callstack.push_back();
                    callstack.back().assign(c1, c2);
                    c1 = c2 + 1;
                }
            }
            callstack.push_back();
            callstack.back() = c1;
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedCallstack, callstack);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugRegisteredGlobalsResult*>(msg.get()))
        {
            AzFramework::ScriptDebugRegisteredGlobalsResult* registeredGlobals = azdynamic_cast<AzFramework::ScriptDebugRegisteredGlobalsResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedRegisteredGlobals, registeredGlobals->m_methods, registeredGlobals->m_properties);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugRegisteredClassesResult*>(msg.get()))
        {
            AzFramework::ScriptDebugRegisteredClassesResult* registeredClasses = azdynamic_cast<AzFramework::ScriptDebugRegisteredClassesResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedRegisteredClasses, registeredClasses->m_classes);
        }
        else if (azrtti_istypeof<AzFramework::ScriptDebugRegisteredEBusesResult*>(msg.get()))
        {
            AzFramework::ScriptDebugRegisteredEBusesResult* registeredEBuses = azdynamic_cast<AzFramework::ScriptDebugRegisteredEBusesResult*>(msg.get());
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnReceivedRegisteredEBuses, registeredEBuses->m_ebusList);
        }
        else
        {
            AZ_Assert(false, "We received a message of an unrecognized class type!");
        }
    }

    void Component::DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID)
    {
        (void)newTargetID;
        (void)oldTargetID;
    }

    void Component::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Component, AZ::Component>()
                ->Version(1)
                ;
        }
    }
}
