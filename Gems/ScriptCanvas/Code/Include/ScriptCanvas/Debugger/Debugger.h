/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Bus.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Network/IRemoteTools.h>
#include <AzFramework/Entity/EntityContextBus.h>

#include <ScriptCanvas/Core/NodeBus.h>

#include "Messages/Request.h"
#include "Messages/Notify.h"
#include "APIArguments.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        //! The ScriptCanvas debugger component, this is the runtime debugger code that directly controls the execution
        //! and provides insight into a running ScriptCanvas graph.
        class ServiceComponent 
            : public AZ::Component
            , public Message::RequestVisitor
            , public ExecutionNotificationsBus::Handler
            , public AZ::SystemTickBus::Handler
        {
           using Lock = AZStd::lock_guard<AZStd::recursive_mutex>;

        public:
            AZ_COMPONENT(ServiceComponent, "{794B1BA5-DE13-46C7-9149-74FFB02CB51B}");

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void Reflect(AZ::ReflectContext* reflection);

            ServiceComponent() = default;
            ServiceComponent(const ServiceComponent&) = delete;
            ~ServiceComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::ServiceComponent
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AZ::SystemTickBus::Handler
            void OnSystemTick() override;
            //////////////////////////////////////////////////////////////////////////

            void OnReceivedMsg(AzFramework::RemoteToolsMessagePointer msg);

            //////////////////////////////////////////////////////////////////////////
            // IRemoteTools handlers
            void RemoteToolsEndpointLeft(const AzFramework::RemoteToolsEndpointInfo& info);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ExecutionNotifications
            void GraphActivated(const GraphActivation&) override;
            void GraphDeactivated(const GraphDeactivation&) override;
            bool IsGraphObserved(const AZ::EntityId& entityId, const GraphIdentifier& identifier) override;
            bool IsVariableObserved(const VariableId& variableId) override;
            void NodeSignaledOutput(const OutputSignal&) override;
            void NodeSignaledInput(const InputSignal&) override;
            void GraphSignaledReturn(const ReturnSignal&) override;
            void NodeStateUpdated(const NodeStateChange&) override;
            void RuntimeError(const ExecutionState& executionState, const AZStd::string_view& description) override;
            void VariableChanged(const VariableChange&) override;
            void AnnotateNode(const AnnotateNodeSignal&) override;
            //////////////////////////////////////////////////////////////////////////

            bool IsAssetObserved(const AZ::Data::AssetId& assetId) const;
            
            //////////////////////////////////////////////////////////////////////////
            // Message processing
            void Visit(Message::AddBreakpointRequest& request) override;
            void Visit(Message::BreakRequest& request) override;
            void Visit(Message::ContinueRequest& request) override;
            void Visit(Message::AddTargetsRequest& request) override;
            void Visit(Message::RemoveTargetsRequest& request) override;
            void Visit(Message::StartLoggingRequest& request) override;
            void Visit(Message::StopLoggingRequest& request) override;
            void Visit(Message::GetAvailableScriptTargets& request) override;
            void Visit(Message::GetActiveEntitiesRequest& request) override;
            void Visit(Message::GetActiveGraphsRequest& request) override;
            void Visit(Message::RemoveBreakpointRequest& request) override;
            void Visit(Message::StepOverRequest& request) override;
            //////////////////////////////////////////////////////////////////////////

        protected:
            void SetTargetsObserved(const TargetEntities& targetEntities, bool observedState);

            template<typename t_SignalType, typename t_MessageType>
            void NodeSignalled(const t_SignalType& nodeSignal)
            {
                auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get();
                if (m_state == SCDebugState_Interactive)
                {
                    SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Interactive: %s", nodeSignal.ToString().data());
                    if (remoteToolsInterface)
                    {
                        remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, t_MessageType(nodeSignal));
                    }
                    Interact();
                }
                else if (m_state == SCDebugState_InteractOnNext)
                {
                    SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("IterateOnNext: %s", nodeSignal.ToString().data());
                    if (remoteToolsInterface)
                    {
                        remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, t_MessageType(nodeSignal));
                    }
                    m_state = SCDebugState_Interactive;
                    Interact();
                }
                else if (m_state == SCDebugState_Attached)
                {
                    const Signal& asSignal(nodeSignal);
                    Breakpoint breakpoint(asSignal);

                    if (m_breakpoints.find(breakpoint) != m_breakpoints.end())
                    {
                        SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Hit breakpoint: %s", nodeSignal.ToString().data());
                        if (remoteToolsInterface)
                        {
                            remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, Message::BreakpointHit(nodeSignal));
                        }
                        m_state = SCDebugState_Interactive;
                        Interact();
                    }
                    else if (m_client.m_script.m_logExecution)
                    {
                        SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Logging Requested: %s", nodeSignal.ToString().data());
                        if (remoteToolsInterface)
                        {
                            remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, t_MessageType(nodeSignal));
                        }
                    }
                }
                else
                {
                    // \todo performance error/warning on receiving this callback
                }
            }

            void Connect(Target& target);
            Message::Request* FilterMessage(AzFramework::RemoteToolsMessagePointer& msg);
            void Interact();
            bool IsAttached() const;
            void ProcessMessages();
            
        private:
            enum eSCDebugState
            {
                SCDebugState_Detached = 0,
                SCDebugState_Attached,
                SCDebugState_Interactive,
                SCDebugState_InteractOnNext,
                SCDebugState_Detaching
            };

            void DisconnectFromClient();

            void RefreshActiveEntityStatus();
            void RefreshActiveGraphStatus();

            AZStd::recursive_mutex m_mutex;
            Target m_self;
            Target m_client;

            AzFramework::EntityContextId m_contextId;

            AZStd::atomic_uint m_state;
            AZStd::unordered_set<Breakpoint> m_breakpoints;

            bool                    m_activeGraphStatusDirty;
            ActiveGraphStatusMap    m_activeGraphs;

            bool                    m_activeEntityStatusDirty;
            ActiveEntityStatusMap   m_activeEntities;

            AZStd::recursive_mutex m_msgMutex;
            AzFramework::RemoteToolsMessageQueue m_msgQueue;
            AzFramework::IRemoteTools* m_remoteTools = nullptr;
            AzFramework::RemoteToolsEndpointStatusEvent::Handler m_endpointLeftEventHandler;
        };
    }
}
