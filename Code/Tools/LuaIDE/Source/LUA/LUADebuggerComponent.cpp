/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUADebuggerComponent.h"
#include "LUAEditorContextMessages.h"

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Script/ScriptContext.h>

#include <AzFramework/Script/ScriptDebugMsgReflection.h>
#include <AzFramework/Script/ScriptRemoteDebuggingConstants.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>


namespace LUADebugger
{
    // Utility functions
    // Returns true if a valid target was found, in which case the info is returned in targetInfo.
    bool GetDesiredTarget(AzFramework::RemoteToolsEndpointInfo& targetInfo)
    {
        // discover what target the user is currently connected to, if any?

        AzFramework::IRemoteTools* remoteTools = AzFramework::RemoteToolsInterface::Get();
        if (!remoteTools)
        {
            return false;
        }

        AzFramework::RemoteToolsEndpointInfo info = remoteTools->GetDesiredEndpoint(AzFramework::LuaToolsKey);
        if (!info.GetPersistentId())
        {
            AZ_TracePrintf("Debug", "The user has not chosen a target to connect to.\n");
            return false;
        }

        targetInfo = info;
        if (!targetInfo.IsValid() || !targetInfo.IsOnline())
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
        m_remoteTools = AzFramework::RemoteToolsInterface::Get();
        LUAEditorDebuggerMessages::Bus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void Component::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        LUAEditorDebuggerMessages::Bus::Handler::BusDisconnect();
        m_remoteTools = nullptr;
    }

    void Component::OnSystemTick()
    {
        if (m_remoteTools)
        {
            const AzFramework::ReceivedRemoteToolsMessages* messages = m_remoteTools->GetReceivedMessages(AzFramework::LuaToolsKey);
            if (messages)
            {
                for (const AzFramework::RemoteToolsMessagePointer& msg : *messages)
                {
                    if (AzFramework::ScriptDebugAck* ack = azdynamic_cast<AzFramework::ScriptDebugAck*>(msg.get()))
                    {
                        if (ack->m_ackCode == AZ_CRC_CE("Ack"))
                        {
                            if (ack->m_request == AZ_CRC_CE("Continue") || ack->m_request == AZ_CRC_CE("StepIn") ||
                                ack->m_request == AZ_CRC_CE("StepOut") || ack->m_request == AZ_CRC_CE("StepOver"))
                            {
                                LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                    &LUAEditor::Context_DebuggerManagement::OnExecutionResumed);
                            }
                            else if (ack->m_request == AZ_CRC_CE("AttachDebugger"))
                            {
                                LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                    &LUAEditor::Context_DebuggerManagement::OnDebuggerAttached);
                            }
                            else if (ack->m_request == AZ_CRC_CE("DetachDebugger"))
                            {
                                LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                    &LUAEditor::Context_DebuggerManagement::OnDebuggerDetached);
                            }
                        }
                        else if (ack->m_ackCode == AZ_CRC_CE("IllegalOperation"))
                        {
                            if (ack->m_request == AZ_CRC_CE("ExecuteScript"))
                            {
                                LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                    &LUAEditor::Context_DebuggerManagement::OnExecuteScriptResult, false);
                            }
                            else if (ack->m_request == AZ_CRC_CE("AttachDebugger"))
                            {
                                LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                    &LUAEditor::Context_DebuggerManagement::OnDebuggerRefused);
                            }
                            else
                            {
                                AZ_TracePrintf(
                                    "LUA Debug",
                                    "Debug Agent: Illegal operation 0x%x. Script context is in the wrong state.\n",
                                    ack->m_request);
                            }
                        }
                        else if (ack->m_ackCode == AZ_CRC_CE("AccessDenied"))
                        {
                            AZ_TracePrintf("LUA Debug", "Debug Agent: Access denied 0x%x. Attach debugger first!\n", ack->m_request);
                            LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                &LUAEditor::Context_DebuggerManagement::OnDebuggerDetached);
                        }
                        else if (ack->m_ackCode == AZ_CRC_CE("InvalidCmd"))
                        {
                            AZ_TracePrintf(
                                "LUA Debug",
                                "The remote script debug agent claims that we sent it an invalid request(0x%x)!\n",
                                ack->m_request);
                        }
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugAckBreakpoint*>(msg.get()))
                    {
                        AzFramework::ScriptDebugAckBreakpoint* ackBreakpoint =
                            azdynamic_cast<AzFramework::ScriptDebugAckBreakpoint*>(msg.get());
                        if (ackBreakpoint->m_id == AZ_CRC_CE("BreakpointHit"))
                        {
                            LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                &LUAEditor::Context_DebuggerManagement::OnBreakpointHit,
                                ackBreakpoint->m_moduleName,
                                ackBreakpoint->m_line);
                        }
                        else if (ackBreakpoint->m_id == AZ_CRC_CE("AddBreakpoint"))
                        {
                            LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                &LUAEditor::Context_DebuggerManagement::OnBreakpointAdded,
                                ackBreakpoint->m_moduleName,
                                ackBreakpoint->m_line);
                        }
                        else if (ackBreakpoint->m_id == AZ_CRC_CE("RemoveBreakpoint"))
                        {
                            LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                                &LUAEditor::Context_DebuggerManagement::OnBreakpointRemoved,
                                ackBreakpoint->m_moduleName,
                                ackBreakpoint->m_line);
                        }
                    }
                    else if (
                        AzFramework::ScriptDebugAckExecute* ackExecute = azdynamic_cast<AzFramework::ScriptDebugAckExecute*>(msg.get()))
                    {
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnExecuteScriptResult, ackExecute->m_result);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugEnumLocalsResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugEnumLocalsResult* enumLocals =
                            azdynamic_cast<AzFramework::ScriptDebugEnumLocalsResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedLocalVariables, enumLocals->m_names);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugEnumContextsResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugEnumContextsResult* enumContexts =
                            azdynamic_cast<AzFramework::ScriptDebugEnumContextsResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedAvailableContexts, enumContexts->m_names);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugGetValueResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugGetValueResult* getValues =
                            azdynamic_cast<AzFramework::ScriptDebugGetValueResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedValueState, getValues->m_value);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugSetValueResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugSetValueResult* setValue =
                            azdynamic_cast<AzFramework::ScriptDebugSetValueResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnSetValueResult, setValue->m_name, setValue->m_result);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugCallStackResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugCallStackResult* callStackResult =
                            azdynamic_cast<AzFramework::ScriptDebugCallStackResult*>(msg.get());
                        AZStd::vector<AZStd::string> callstack;
                        const char* c1 = callStackResult->m_callstack.c_str();
                        for (const char* c2 = c1; *c2; ++c2)
                        {
                            if (*c2 == '\n')
                            {
                                callstack.emplace_back().assign(c1, c2);
                                c1 = c2 + 1;
                            }
                        }
                        callstack.emplace_back() = c1;
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedCallstack, callstack);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugRegisteredGlobalsResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugRegisteredGlobalsResult* registeredGlobals =
                            azdynamic_cast<AzFramework::ScriptDebugRegisteredGlobalsResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedRegisteredGlobals,
                            registeredGlobals->m_methods,
                            registeredGlobals->m_properties);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugRegisteredClassesResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugRegisteredClassesResult* registeredClasses =
                            azdynamic_cast<AzFramework::ScriptDebugRegisteredClassesResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedRegisteredClasses, registeredClasses->m_classes);
                    }
                    else if (azrtti_istypeof<AzFramework::ScriptDebugRegisteredEBusesResult*>(msg.get()))
                    {
                        AzFramework::ScriptDebugRegisteredEBusesResult* registeredEBuses =
                            azdynamic_cast<AzFramework::ScriptDebugRegisteredEBusesResult*>(msg.get());
                        LUAEditor::Context_DebuggerManagement::Bus::Broadcast(
                            &LUAEditor::Context_DebuggerManagement::OnReceivedRegisteredEBuses, registeredEBuses->m_ebusList);
                    }
                    else
                    {
                        AZ_Assert(false, "We received a message of an unrecognized class type!");
                    }
                }
                m_remoteTools->ClearReceivedMessages(AzFramework::LuaToolsKey);
            }
        }
    }

    void Component::EnumerateContexts()
    {
        AZ_TracePrintf("LUA Debug", "Component::EnumerateContexts()\n");

        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("EnumContexts")));
        }
    }

    void Component::AttachDebugger(const char* scriptContextName)
    {
        AZ_TracePrintf("LUA Debug", "Component::AttachDebugger( %s )\n", scriptContextName);

        AZ_Assert(scriptContextName, "You need to supply a valid script context name to attach to!");
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("AttachDebugger"), scriptContextName));
        }
    }

    void Component::DetachDebugger()
    {
        AZ_TracePrintf("LUA Debug", "Component::DetachDebugger()\n");

        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("DetachDebugger")));
        }
    }

    void Component::EnumRegisteredClasses(const char* scriptContextName)
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("EnumRegisteredClasses"), scriptContextName));
        }
    }

    void Component::EnumRegisteredEBuses(const char* scriptContextName)
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("EnumRegisteredEBuses"), scriptContextName));
        }
    }

    void Component::EnumRegisteredGlobals(const char* scriptContextName)
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("EnumRegisteredGlobals"), scriptContextName));
        }
    }

    void Component::CreateBreakpoint(const AZStd::string& debugName, int lineNumber)
    {
        // register a breakpoint.

        // Debug name will be the full, absolute path, so convert it to the relative path
        AZStd::string relativePath = debugName;
        AzToolsFramework::AssetSystemRequestBus::Broadcast(
            &AzToolsFramework::AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, debugName, relativePath);
        relativePath = "@" + relativePath;

        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            // local editors are never debuggable (they'd never have the debuggable flag) so if you get here you know its over the network
            // and its network id is targetID.
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugBreakpointRequest(AZ_CRC_CE("AddBreakpoint"), relativePath.c_str(), static_cast<AZ::u32>(lineNumber)));
        }
    }

    void Component::RemoveBreakpoint(const AZStd::string& debugName, int lineNumber)
    {
        // remove a breakpoint.

        // Debug name will be the full, absolute path, so convert it to the relative path
        AZStd::string relativePath = debugName;
        AzToolsFramework::AssetSystemRequestBus::Broadcast(
            &AzToolsFramework::AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, debugName, relativePath);
        relativePath = "@" + relativePath;

        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            // local editors are never debuggable (they'd never have the debuggable flag) so if you get here you know its over the network
            // and its network id is targetID.
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugBreakpointRequest(AZ_CRC_CE("RemoveBreakpoint"), relativePath.c_str(), static_cast<AZ::u32>(lineNumber)));
        }
    }

    void Component::DebugRunStepOver()
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("StepOver")));
        }
    }

    void Component::DebugRunStepIn()
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("StepIn")));
        }
    }

    void Component::DebugRunStepOut()
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("StepOut")));
        }
    }

    void Component::DebugRunStop()
    {
        // Script context can't be stopped. What should we do here?
    }

    void Component::DebugRunContinue()
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("Continue")));
        }
    }

    void Component::EnumLocals()
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("EnumLocals")));
        }
    }

    void Component::GetValue(const AZStd::string& varName)
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("GetValue"), varName.c_str()));
        }
    }

    void Component::SetValue(const AZ::ScriptContextDebug::DebugValue& value)
    {
        (void)value;
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            AzFramework::ScriptDebugSetValue request;
            request.m_value = value;
            m_remoteTools->SendRemoteToolsMessage(targetInfo, request);
        }
    }

    void Component::GetCallstack()
    {
        AzFramework::RemoteToolsEndpointInfo targetInfo;
        if (m_remoteTools && GetDesiredTarget(targetInfo))
        {
            m_remoteTools->SendRemoteToolsMessage(targetInfo, AzFramework::ScriptDebugRequest(AZ_CRC_CE("GetCallstack")));
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
