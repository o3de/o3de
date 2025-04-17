/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ClientTransceiver.h"

#include "Messages/Request.h"
#include "Messages/Notify.h"

#include <ScriptCanvas/Utils/ScriptCanvasConstants.h>

using namespace AzFramework;

namespace ScriptCanvas
{
    namespace Debugger
    {
        ClientTransceiver::ClientTransceiver()
        {
            ClientRequestsBus::Handler::BusConnect();
            ClientUIRequestBus::Handler::BusConnect();
            AZ::SystemTickBus::Handler::BusConnect();

            DiscoverNetworkTargets();

            for (auto& idAndInfo : m_networkTargets)
            {
                if (idAndInfo.second.IsSelf())
                {
                    m_selfTarget = idAndInfo.second;
                    SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Self found!");
                    break;
                }
            }

            if (m_selfTarget.IsValid())
            { 
                m_currentTarget = m_selfTarget;
                DesiredTargetConnected(true);
            }
            else
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Self NOT found!");
            }

            m_addCache.m_logExecution = true;
            m_removeCache.m_logExecution = false;
        }

        ClientTransceiver::~ClientTransceiver()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            ClientUIRequestBus::Handler::BusDisconnect();
            ClientRequestsBus::Handler::BusDisconnect();
        }

        void ClientTransceiver::AddBreakpoint(const Breakpoint& breakpoint)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX sending AddBreakpoint Request %s", breakpoint.ToString().data());
            if (AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get())
            {
                remoteTools->SendRemoteToolsMessage(m_currentTarget, Message::AddBreakpointRequest(breakpoint));
            }
        }

        void ClientTransceiver::AddVariableChangeBreakpoint(const VariableChangeBreakpoint&)
        {

        }

        void ClientTransceiver::Break()
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX Sending Break Request %s", m_currentTarget.GetDisplayName());
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::BreakRequest());
        }
        
        void ClientTransceiver::BreakpointAdded(const Breakpoint& breakpoint)
        {
            Lock lock(m_mutex);
            auto inactiveIter = m_breakpointsInactive.find(breakpoint);

            if (inactiveIter != m_breakpointsInactive.end())
            {
                m_breakpointsInactive.erase(inactiveIter);
            }

            if (m_breakpointsActive.find(breakpoint) == m_breakpointsActive.end())
            {
                m_breakpointsActive.insert(breakpoint);
                ServiceNotificationsBus::Broadcast(&ServiceNotifications::BreakPointAdded, breakpoint);
            }
        }
        void ClientTransceiver::ClearMessages()
        {
            Lock guard(m_msgMutex);
            m_msgQueue.clear();
        }

        void ClientTransceiver::Continue()
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX Sending Continue Request %s", m_currentTarget.GetDisplayName());
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::ContinueRequest());
        }

        void ClientTransceiver::DesiredTargetConnected(bool connected)
        {
            if (connected)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("DesiredTarget connected!, sending connect request to %s", m_currentTarget.GetDisplayName());
            }
            else
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("DesiredTarget NOT connected!");
                m_currentTarget = AzFramework::RemoteToolsEndpointInfo();
            }

            ClientUINotificationBus::Broadcast(&ClientUINotifications::OnCurrentTargetChanged);
            
            if (m_currentTarget.IsValid())
            {
                RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::ConnectRequest(m_connectionState));
            }
        }

        void ClientTransceiver::DesiredTargetChanged([[maybe_unused]] AZ::u32 newId, [[maybe_unused]] AZ::u32 oldId)
        {
            if (HasValidConnection())
            {
                DisconnectFromTarget();
            }
        }
        
        AzFramework::RemoteToolsEndpointContainer ClientTransceiver::EnumerateAvailableNetworkTargets()
        {
            return m_networkTargets;
        }

        void ClientTransceiver::DiscoverNetworkTargets()
        {
            AzFramework::RemoteToolsEndpointContainer targets;
            if (AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get())
            {
                remoteTools->EnumTargetInfos(ScriptCanvas::RemoteToolsKey, targets);
            }

            AzFramework::RemoteToolsEndpointContainer connectableTargets;
            
            for (auto& idAndInfo : targets)
            {
                const auto& targetInfo = idAndInfo.second;
                auto isConnectable = IsTargetConnectable(targetInfo);
                if (isConnectable.IsSuccess())
                {
                    connectableTargets[idAndInfo.first] = idAndInfo.second;
                    SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Debugger TRX can connect to %s", targetInfo.GetDisplayName());
                }
                else
                {
                    SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Debugger TRX can't connect to %s because: %s", targetInfo.GetDisplayName(), isConnectable.GetError().c_str());
                }
            }

            {
                Lock lock(m_mutex);
                m_networkTargets = connectableTargets;
            }
        }

        AzFramework::RemoteToolsEndpointInfo ClientTransceiver::GetNetworkTarget()
        {
            AzFramework::RemoteToolsEndpointInfo targetInfo;
            if (AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get())
            {
                targetInfo = remoteTools->GetDesiredEndpoint(ScriptCanvas::RemoteToolsKey);
            }

            if (!targetInfo.GetPersistentId())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Debugger TRX The user has not chosen a target to connect to.\n");
                return AzFramework::RemoteToolsEndpointInfo();
            }

            auto isConnectable = IsTargetConnectable(targetInfo);
            if (!isConnectable.IsSuccess())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Debugger TRX has no target because: %s", isConnectable.GetError().c_str());
                return AzFramework::RemoteToolsEndpointInfo();
            }

            return targetInfo;
        }
        
        void ClientTransceiver::GetAvailableScriptTargets()
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX sending GetAvailableScriptTargets Request %s", m_currentTarget.GetDisplayName());
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::GetAvailableScriptTargets());
        }
        
        void ClientTransceiver::GetActiveEntities()
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX sending GetActiveEntities Request %s", m_currentTarget.GetDisplayName());
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::GetActiveEntitiesRequest());
        }

        void ClientTransceiver::GetActiveGraphs()
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX sending GetActiveGraphs Request %s", m_currentTarget.GetDisplayName());
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::GetActiveGraphsRequest());
        }
        
        void ClientTransceiver::GetVariableValue()
        {

        }

        bool ClientTransceiver::HasValidConnection() const
        {
            return m_currentTarget.IsValid();
        }

        bool ClientTransceiver::IsConnected(const AzFramework::RemoteToolsEndpointInfo& targetInfo) const
        {
            return m_currentTarget.IsIdentityEqualTo(targetInfo);
        }

        bool ClientTransceiver::IsConnectedToSelf() const
        {
            return IsConnected(m_selfTarget) || !m_currentTarget.IsValid();
        }

        void ClientTransceiver::OnReceivedMsg(AzFramework::RemoteToolsMessagePointer msg)
        {
            {
                Lock lock(m_msgMutex);
                if (msg)
                {
                    m_msgQueue.push_back(msg);
                }
                else
                {
                    AZ_Error("ScriptCanvas Debugger", msg, "We received a NULL message in the trx message queue!");
                }
            }

            ProcessMessages();
        }
        
        void ClientTransceiver::ProcessMessages()
        {
            AzFramework::RemoteToolsMessageQueue messages;
            
            while (true)
            {
                {
                    Lock lock(m_msgMutex);
                    
                    if (m_msgQueue.empty())
                    {
                        return;
                    }
                    
                    AZStd::swap(messages, m_msgQueue);
                }
                
                while (!messages.empty())
                {
                    AzFramework::RemoteToolsMessagePointer msg = *messages.begin();
                    messages.pop_front();

                    if (auto request = azrtti_cast<Message::Notification*>(msg.get()))
                    {
                        request->Visit(*this);
                    }
                }
            }
        }

        void ClientTransceiver::DisconnectFromTarget()
        {
            if (AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get())
            {
                remoteTools->SendRemoteToolsMessage(m_currentTarget, Message::DisconnectRequest());
            }
        }

        void ClientTransceiver::CleanupConnection()
        {
            ClearMessages();
        }

        void ClientTransceiver::RemoveBreakpoint(const Breakpoint&)
        {

        }

        void ClientTransceiver::RemoveVariableChangeBreakpoint(const VariableChangeBreakpoint&)
        {

        }

        void ClientTransceiver::SetVariableValue()
        {

        }

        void ClientTransceiver::StepOver()
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("TRX Sending StepOver Request %s", m_currentTarget.GetDisplayName());
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::StepOverRequest());
        }

        void ClientTransceiver::TargetJoinedNetwork(AzFramework::RemoteToolsEndpointInfo info)
        {
            auto isConnectable = IsTargetConnectable(info);
            if (isConnectable.IsSuccess())
            {
                m_networkTargets.emplace(info.GetPersistentId(), info);
                ServiceNotificationsBus::Broadcast(&ServiceNotifications::BecameUnavailable, Target(info));
            }
        }

        void ClientTransceiver::TargetLeftNetwork(AzFramework::RemoteToolsEndpointInfo info)
        {
            bool eventNeeded = false;

            if (info.IsIdentityEqualTo(m_currentTarget))
            {
                CleanupConnection();
                eventNeeded = true;
            }
            else
            {
                auto iter = m_networkTargets.find(info.GetPersistentId());
                if (iter != m_networkTargets.end())
                {
                    m_networkTargets.erase(iter);
                    eventNeeded = true;
                }
            }

            if (eventNeeded)
            {
                ServiceNotificationsBus::Broadcast(&ServiceNotifications::BecameUnavailable, Target(info));
            }
        }

        void ClientTransceiver::Visit(Message::AvailableScriptTargetsResult& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received AvailableScriptTargetsResult!");
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::GetAvailableScriptTargetResult, notification.m_payload);
        }
        
        void ClientTransceiver::Visit(Message::ActiveEntitiesResult& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received ActiveEntitiesResult!");
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::GetActiveEntitiesResult, notification.m_payload);
        }
        
        void ClientTransceiver::Visit(Message::ActiveGraphsResult& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received ActiveGraphsResult!");
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::GetActiveGraphsResult, notification.m_payload);
        }

        void ClientTransceiver::Visit(Message::AnnotateNode& notification)
        {
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::AnnotateNode, notification.m_payload);
        }
        
        void ClientTransceiver::Visit(Message::BreakpointAdded& notification)
        {
            BreakpointAdded(notification.m_breakpoint);
        }
        
        void ClientTransceiver::Visit(Message::BreakpointHit& notification)
        {
            BreakpointAdded(notification.m_breakpoint);
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::BreakPointHit, notification.m_breakpoint);
        }

        void ClientTransceiver::Visit(Message::Connected& notification)
        {
            if (notification.m_target.m_info.IsIdentityEqualTo(m_currentTarget))
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Neat. we're connected");
                ServiceNotificationsBus::Broadcast(&ServiceNotifications::Connected, notification.m_target);
            }
            else
            {
                AZ_Warning("ScriptCanvas Debugger", false, "Received connection notification, but targets did not match");
            }
        }

        void ClientTransceiver::Visit([[maybe_unused]] Message::Disconnected& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Disconnect Notification Received");
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::Disconnected);

            CleanupConnection();

            if (m_resetDesiredTarget)
            {
                m_resetDesiredTarget = false;
                RemoteToolsInterface::Get()->SetDesiredEndpointInfo(ScriptCanvas::RemoteToolsKey, m_previousDesiredInfo);
                m_currentTarget = AzFramework::RemoteToolsEndpointInfo();
            }
        }

        void ClientTransceiver::Visit([[maybe_unused]] Message::Continued& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received continue notification!");
            Target connectedTarget;
            connectedTarget.m_info = m_currentTarget;

            ServiceNotificationsBus::Broadcast(&ServiceNotifications::Continued, connectedTarget);
        }

        void ClientTransceiver::Visit(Message::GraphActivated& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received GraphActivated! %s", notification.m_payload.ToString().data());
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::GraphActivated, notification.m_payload);
        }

        void ClientTransceiver::Visit(Message::GraphDeactivated& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received GraphDeactivated! %s", notification.m_payload.ToString().data());
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::GraphDeactivated, notification.m_payload);
        }

        void ClientTransceiver::Visit(Message::SignaledInput& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received input signal notification! %s", notification.m_signal.ToString().data());
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::SignaledInput, notification.m_signal);
        }

        void ClientTransceiver::Visit(Message::SignaledOutput& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received output signal notification! %s", notification.m_signal.ToString().data());
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::SignaledOutput, notification.m_signal);
        }

        void ClientTransceiver::Visit(Message::VariableChanged& notification)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("received variable change notification! %s", notification.m_variableChange.ToString().data());
            ServiceNotificationsBus::Broadcast(&ServiceNotifications::VariableChanged, notification.m_variableChange);
        }

        void ClientTransceiver::OnSystemTick()
        {
            AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get();
            if (!remoteTools)
                return;

            if (!m_addCache.m_entities.empty())
            {
                remoteTools->SendRemoteToolsMessage(m_currentTarget, Message::AddTargetsRequest(m_addCache));
                m_connectionState.Merge(m_addCache);
                m_addCache.Clear();
            }

            if (!m_removeCache.m_entities.empty())
            {
                remoteTools->SendRemoteToolsMessage(m_currentTarget, Message::RemoveTargetsRequest(m_removeCache));
                m_connectionState.Remove(m_removeCache);
                m_removeCache.Clear();
            }

            const AzFramework::ReceivedRemoteToolsMessages* messages = remoteTools->GetReceivedMessages(ScriptCanvas::RemoteToolsKey);
            if (messages)
            {
                for (const AzFramework::RemoteToolsMessagePointer& msg : *messages)
                {
                    OnReceivedMsg(msg);
                }
                remoteTools->ClearReceivedMessagesForNextTick(ScriptCanvas::RemoteToolsKey);
            }
        }

        void ClientTransceiver::StartEditorSession()
        {
            if (!m_currentTarget.IsValid())
            {
                m_resetDesiredTarget = true;
                if (AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get())
                {
                    m_previousDesiredInfo = remoteTools->GetDesiredEndpoint(ScriptCanvas::RemoteToolsKey);
                    remoteTools->SetDesiredEndpointInfo(ScriptCanvas::RemoteToolsKey, m_selfTarget);
                }
            }
        }

        void ClientTransceiver::StopEditorSession()
        {
            if (m_resetDesiredTarget)
            {
                DisconnectFromTarget();
            }
        }

        void ClientTransceiver::StartLogging(ScriptTarget& initialTargets)
        {
            m_connectionState.m_logExecution = true;
            m_connectionState.Clear();
            m_connectionState.Merge(initialTargets);

            if (m_currentTarget.IsValid())
            {
                RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_currentTarget, Message::StartLoggingRequest(initialTargets));
            }
        }

        void ClientTransceiver::StopLogging()
        {
            if (AzFramework::IRemoteTools* remoteTools = RemoteToolsInterface::Get())
            {
                remoteTools->SendRemoteToolsMessage(m_currentTarget, Message::StopLoggingRequest());
            }

            m_connectionState.m_logExecution = false;
            m_connectionState.Clear();
        }

        void ClientTransceiver::AddEntityLoggingTarget(const AZ::EntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
        {
            auto insertResult = m_addCache.m_entities.insert(entityId);
            auto addCacheIter = insertResult.first;

            addCacheIter->second.insert(graphIdentifier);

            auto removeCacheIter = m_removeCache.m_entities.find(entityId);

            if (removeCacheIter != m_removeCache.m_entities.end())
            {
                removeCacheIter->second.erase(graphIdentifier);
            }
        }

        void ClientTransceiver::RemoveEntityLoggingTarget(const AZ::EntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
        {
            auto insertResult = m_removeCache.m_entities.insert(entityId);
            auto removeCacheIter = insertResult.first;

            removeCacheIter->second.insert(graphIdentifier);

            auto addCacheIter = m_addCache.m_entities.find(entityId);

            if (addCacheIter != m_addCache.m_entities.end())
            {
                addCacheIter->second.erase(graphIdentifier);
            }
        }

        void ClientTransceiver::AddGraphLoggingTarget(const AZ::Data::AssetId& assetId)
        {
            m_addCache.m_graphs.insert(assetId);
            m_removeCache.m_graphs.erase(assetId);
        }

        void ClientTransceiver::RemoveGraphLoggingTarget(const AZ::Data::AssetId& assetId)
        {
            m_addCache.m_graphs.erase(assetId);
            m_removeCache.m_graphs.insert(assetId);
        }
    }
}
