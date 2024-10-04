/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/GameEntityContextBus.h>

#include "Debugger.h"
#include "Messages/Notify.h"
#include "Messages/Request.h"

#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Utils/ScriptCanvasConstants.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        void ServiceComponent::Connect(Target& target)
        {
            // \todo disconnect previous or reject or something
            m_client = target;

            if (m_state == SCDebugState_Detached)
            {
                m_state = SCDebugState_Attached;
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Debugger attached to new connection");
            }
            else if (m_state == SCDebugState_Interactive)
            {
                Lock guard(m_msgMutex);
                m_msgQueue.push_back(AzFramework::RemoteToolsMessagePointer(aznew Message::ContinueRequest()));
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Debugger attached to new connection, continuing");
            }
            else
            {
                AZ_Warning("ScriptCanvas Debugger", false, "Something has gone terribly wrong with the debugger");
                return;
            }

            m_self.CopyScript(m_client);

            m_activeEntityStatusDirty = true;
            m_activeGraphStatusDirty = true;

            auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get();
            if (remoteToolsInterface)
            {
                remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, Message::Connected(m_self));
            }
        }

        Message::Request* ServiceComponent::FilterMessage(AzFramework::RemoteToolsMessagePointer& msg)
        {
            AzFramework::RemoteToolsEndpointInfo client =
                AzFramework::RemoteToolsInterface::Get()->GetEndpointInfo(ScriptCanvas::RemoteToolsKey, msg->GetSenderTargetId());

            // cull messages without a target match
            if (!m_client.m_info.IsIdentityEqualTo(client))
            {
                if (auto connection = azrtti_cast<Message::ConnectRequest*>(msg.get()))
                {
                    // connection messages are handled separately
                    Target connectionTarget;
                    connectionTarget.m_info = client;
                    connectionTarget.m_script = connection->m_target;
                    Connect(connectionTarget);
                    return nullptr;
                }

                // \todo send a connection denied message
                // remoteToolsInterface->SendRemoteToolsMessage(sender, Message::Denied());
                return nullptr;
            }
            else if (azrtti_cast<Message::DisconnectRequest*>(msg.get()))
            {
                DisconnectFromClient();
            }

            return azrtti_cast<Message::Request*>(msg.get());
        }

        void ServiceComponent::Interact()
        {
            if (m_state == SCDebugState_Interactive)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is going into interactive mode");

                while (true)
                {
                    // the events aren't getting dispatched from another thread, so force them out here
                    //remoteToolsInterface->DispatchMessages(k_clientRequestsMsgSlotId);

                    // process any new ones
                    ProcessMessages();

                    // check if we're still in interactive mode
                    if (m_state != SCDebugState_Interactive)
                    {
                        return;
                    }

                    // give other threads a chance
                    AZStd::this_thread::yield();
                }
            }
        }

        bool ServiceComponent::IsAttached() const
        {
            return m_state != SCDebugState_Detached && m_state != SCDebugState_Detaching;
        }

        void ServiceComponent::ProcessMessages()
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

                    if (Message::Request* request = FilterMessage(msg))
                    {
                        request->Visit(*this);
                    }
                }
            }
        }

        void ServiceComponent::OnSystemTick()
        {
            AzFramework::IRemoteTools* remoteTools = AzFramework::RemoteToolsInterface::Get();
            if (remoteTools)
            {
                const AzFramework::ReceivedRemoteToolsMessages* messages =
                    remoteTools->GetReceivedMessages(ScriptCanvas::RemoteToolsKey);
                if (messages)
                {
                    for (const AzFramework::RemoteToolsMessagePointer& msg : *messages)
                    {
                        OnReceivedMsg(msg);
                    }
                    remoteTools->ClearReceivedMessagesForNextTick(ScriptCanvas::RemoteToolsKey);
                }
            }
        }

        void ServiceComponent::OnReceivedMsg(AzFramework::RemoteToolsMessagePointer msg)
        {
            if (!msg)
            {
                AZ_Error("ScriptCanvas Debugger", false, "We received a NULL message in the service message queue");
                return;
            }

            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("service component received a message of type: %s", msg->RTTI_GetTypeName());

            if (m_state != SCDebugState_Detaching && azrtti_cast<Message::Request*>(msg.get()))
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("service is putting the request in the queue");

                {
                    Lock lock(m_msgMutex);
                    m_msgQueue.push_back(msg);
                }

                if (m_state != SCDebugState_Interactive)
                {
                    ProcessMessages();
                }
            }
            else
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("service is rejecting the message");
                // \todo send a connection denied message
                // remoteToolsInterface->SendRemoteToolsMessage(sender, Message::Acknowledge(0, AZ_CRC_CE("AccessDenied")));
            }
        }

        void ServiceComponent::RemoteToolsEndpointLeft(const AzFramework::RemoteToolsEndpointInfo& info)
        {
            if (m_client.m_info.IsIdentityEqualTo(info))
            {
                DisconnectFromClient();
            }
        }

        void ServiceComponent::Init()
        {}

        void ServiceComponent::Activate()
        {
            m_state = SCDebugState_Detached;
            ExecutionNotificationsBus::Handler::BusConnect();
            AZ::SystemTickBus::Handler::BusConnect();

            m_remoteTools = AzFramework::RemoteToolsInterface::Get();
            if (!m_remoteTools)
                return;

            m_endpointLeftEventHandler = AzFramework::RemoteToolsEndpointStatusEvent::Handler(
                [this](AzFramework::RemoteToolsEndpointInfo info)
                {
                    this->RemoteToolsEndpointLeft(info);
                });
            m_remoteTools->RegisterRemoteToolsEndpointLeftHandler(ScriptCanvas::RemoteToolsKey, m_endpointLeftEventHandler);

            AzFramework::RemoteToolsEndpointContainer targets;
            m_remoteTools->EnumTargetInfos(ScriptCanvas::RemoteToolsKey, targets);
            for (auto& idAndInfo : targets)
            {
                if (idAndInfo.second.IsSelf())
                {
                    m_self.m_info = idAndInfo.second;
                    SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Self found!");
                }
            }

            if (!m_self.m_info.IsValid())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Self NOT found!");
            }
        }

        void ServiceComponent::Deactivate()
        {
            m_remoteTools = nullptr;
            if (m_state != SCDebugState_Detached)
            {
                //\todo DetachAll();
            }

            ExecutionNotificationsBus::Handler::BusDisconnect();

            {
                Lock lock(m_msgMutex);
                m_msgQueue.clear();
            }
        }

        void ServiceComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ScriptCanvasDebugService"));
        }

        void ServiceComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ScriptCanvasDebugService"));
        }

        void ServiceComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("ScriptCanvasService"));
        }

        void ServiceComponent::Reflect(AZ::ReflectContext* context)
        {
            ReflectExecutionBusArguments(context);
            ReflectArguments(context);
            ReflectNotifications(context);
            ReflectRequests(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ServiceComponent, AZ::Component>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ServiceComponent>("Script Canvas Runtime Debugger", "Provides remote debugging services for Script Canvas")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                        ;
                }
            }
        }

        void ServiceComponent::GraphActivated(const GraphActivation& graphInfo)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("GraphActivation: %s", graphInfo.ToString().data());

            Lock lock(m_mutex);

            ActiveGraphStatus* activeGraphStatus = nullptr;

            auto graphIter = m_activeGraphs.find(graphInfo.m_graphIdentifier.m_assetId);

            if (graphIter == m_activeGraphs.end())
            {
                activeGraphStatus = &(m_activeGraphs[graphInfo.m_graphIdentifier.m_assetId]);

                if (IsAssetObserved(graphInfo.m_graphIdentifier.m_assetId))
                {
                    activeGraphStatus->m_isObserved = true;
                }
            }
            else
            {
                activeGraphStatus = &(graphIter->second);
            }

            if (activeGraphStatus)
            {
                activeGraphStatus->m_instanceCounter++;
            }

            const AZ::NamedEntityId& namedEntityId = graphInfo.m_runtimeEntity;

            auto entityIter = m_activeEntities.find(namedEntityId);

            ActiveEntityStatus* entityStatus = nullptr;

            if (entityIter == m_activeEntities.end())
            {
                entityStatus = &(m_activeEntities[namedEntityId]);
                entityStatus->m_namedEntityId = namedEntityId;
            }
            else
            {
                entityStatus = &(entityIter->second);
            }

            if (entityStatus)
            {
                auto graphCountIter = entityStatus->m_activeGraphs.find(graphInfo.m_graphIdentifier);
                if (graphCountIter == entityStatus->m_activeGraphs.end())
                {
                    ActiveGraphStatus graphStatus;
                    graphStatus.m_instanceCounter = 1;

                    graphStatus.m_isObserved = IsGraphObserved(graphInfo.m_runtimeEntity, graphInfo.m_graphIdentifier);
                    entityStatus->m_activeGraphs.insert(AZStd::make_pair(graphInfo.m_graphIdentifier, graphStatus));
                }
                else
                {
                    // We received a double activation signal for the same graph without a deactivate?
                    SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Accounting error. An activated graph was already found for the entity the active list");
                }
            }

            if (!m_client.m_script.m_staticEntities.empty())
            {
                auto clientStaticEntityIter = m_client.m_script.m_staticEntities.begin();

                while (clientStaticEntityIter != m_client.m_script.m_staticEntities.end())
                {
                    AZ::EntityId runtimeEntityId;

                    AZ::SliceComponent::EntityIdToEntityIdMap loadedEntityIdMap;
                    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(runtimeEntityId, m_contextId, &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::FindLoadedEntityIdMapping, clientStaticEntityIter->first);

                    if (runtimeEntityId == graphInfo.m_runtimeEntity)
                    {
                        AZ::EntityId staticEntity = clientStaticEntityIter->first;

                        // Remap the static entity identifiers and move them into the 'self' script targets
                        {
                            auto insertResult = m_self.m_script.m_entities.insert(runtimeEntityId);
                            auto selfStaticEntityIter = m_self.m_script.m_staticEntities.find(staticEntity);
                            AZ_Assert(selfStaticEntityIter != m_self.m_script.m_staticEntities.end(), "self scripts miss match with client scripts");
                            // Once debugger supports multiple same graphs per entity, we can directly copy over without comparing,
                            // because the component id should be hooked between editor and runtime component
                            for (auto graphIdentifier : selfStaticEntityIter->second)
                            {
                                if (graphIdentifier.m_assetId == graphInfo.m_graphIdentifier.m_assetId)
                                {
                                    insertResult.first->second.insert(graphInfo.m_graphIdentifier);
                                    selfStaticEntityIter->second.erase(graphIdentifier);
                                    break;
                                }
                            }
                            if (selfStaticEntityIter->second.empty())
                            {
                                m_self.m_script.m_staticEntities.erase(staticEntity);
                            }
                        }

                        // Remap the static entity identifiers and move them into the 'client' script targets
                        {
                            auto insertResult = m_client.m_script.m_entities.insert(runtimeEntityId);
                            // Once debugger supports multiple same graphs per entity, we can directly copy over without comparing,
                            // because the component id should be hooked between editor and runtime component
                            for (auto graphIdentifier : clientStaticEntityIter->second)
                            {
                                if (graphIdentifier.m_assetId == graphInfo.m_graphIdentifier.m_assetId)
                                {
                                    insertResult.first->second.insert(graphInfo.m_graphIdentifier);
                                    clientStaticEntityIter->second.erase(graphIdentifier);
                                    break;
                                }
                            }
                            if (clientStaticEntityIter->second.empty())
                            {
                                clientStaticEntityIter = m_client.m_script.m_staticEntities.erase(clientStaticEntityIter);
                            }
                        }
                        break;
                    }
                    else
                    {
                        ++clientStaticEntityIter;
                    }
                }
            }

            GraphActivation payload = graphInfo;
            payload.m_entityIsObserved = IsGraphObserved(graphInfo.m_runtimeEntity, graphInfo.m_graphIdentifier);
            if (m_remoteTools)
            {
                m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::GraphActivated(payload));
            }
        }

        void ServiceComponent::GraphDeactivated(const GraphDeactivation& graphInfo)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("GraphDeactivated: %s", graphInfo.ToString().data());

            Lock lock(m_mutex);

            auto graphIter = m_activeGraphs.find(graphInfo.m_graphIdentifier.m_assetId);
            if (graphIter == m_activeGraphs.end())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Accounting error. A deactivated graph was not found in the active list");
            }
            else
            {
                --graphIter->second.m_instanceCounter;

                if (graphIter->second.m_instanceCounter == 0)
                {
                    m_activeGraphs.erase(graphIter);
                }
            }

            auto namedEntity = graphInfo.m_runtimeEntity;

            auto entityIter = m_activeEntities.find(namedEntity);
            if (entityIter == m_activeEntities.end())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Accounting error. A deactivated entity was not found in the active list");
            }
            else
            {
                ActiveEntityStatus& entityStatus = entityIter->second;

                auto graphIterator = entityStatus.m_activeGraphs.find(graphInfo.m_graphIdentifier);

                if (graphIterator == entityStatus.m_activeGraphs.end())
                {
                    SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Accounting error. A deactivated graph was not found for the entity the active list");
                }
                else
                {
                    entityStatus.m_activeGraphs.erase(graphIterator);
                }

                if (entityStatus.m_activeGraphs.empty())
                {
                    // At this point entityStatus is bad. So no touchy
                    m_activeEntities.erase(entityIter);
                }
            }

            GraphActivation payload = graphInfo;
            payload.m_entityIsObserved = IsGraphObserved(graphInfo.m_runtimeEntity, graphInfo.m_graphIdentifier);

            if (m_remoteTools)
            {
                m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::GraphDeactivated(payload));
            }
        }

        bool ServiceComponent::IsAssetObserved(const AZ::Data::AssetId& assetId) const
        {
            if (!m_client.m_script.m_logExecution)
            {
                return false;
            }

#if defined(SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING)
            return true;
#else
            if (m_state == SCDebugState_Detached || m_state == SCDebugState_Detaching)
            {
                return false;
            }

            return m_client.m_script.IsObservingAsset(assetId);
#endif //defined(SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING)
        }

        bool ServiceComponent::IsGraphObserved(const AZ::EntityId& entityId, const GraphIdentifier& identifier)
        {
            if (!m_client.m_script.m_logExecution)
            {
                return false;
            }

#if defined(SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING)
            return true;
#else
            if (m_state == SCDebugState_Detached || m_state == SCDebugState_Detaching)
            {
                return false;
            }

            return m_client.m_script.IsObserving(entityId, identifier);
#endif //defined(SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING)
        }

        bool ServiceComponent::IsVariableObserved(const VariableId& /*variableId*/)
        {
#if defined(SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING)
            return true;
#else
            // \todo finish me!
            return true;
#endif//defined(SCRIPT_CANVAS_DEBUGGER_IS_ALWAYS_OBSERVING)
        }

        void ServiceComponent::NodeSignaledOutput(const OutputSignal& nodeSignal)
        {
            NodeSignalled<OutputSignal, Message::SignaledOutput>(nodeSignal);
        }

        void ServiceComponent::NodeSignaledInput(const InputSignal& nodeSignal)
        {
            NodeSignalled<InputSignal, Message::SignaledInput>(nodeSignal);
        }

        void ServiceComponent::GraphSignaledReturn([[maybe_unused]] const ReturnSignal& graphSignal)
        {
        }

        void ServiceComponent::NodeStateUpdated(const NodeStateChange&)
        {
            // \todo decide whether or not this should break
            //             if (m_state == SCDebugState_Interactive)
            //             {
            //                 Interact();
            //             }
        }

        void ServiceComponent::VariableChanged(const VariableChange& variableChange)
        {
            /*
            if (variableChange breakpoint hit)
            {}
            else
            */

            if (m_client.m_script.m_logExecution || m_state == SCDebugState_Interactive || m_state == SCDebugState_InteractOnNext)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("Interactive: %s", variableChange.ToString().data());
                if (m_remoteTools)
                {
                    m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::VariableChanged(variableChange));
                }

                if (m_state == SCDebugState_Interactive || m_state == SCDebugState_InteractOnNext)
                {
                    Interact();
                }
            }
        }

        void ServiceComponent::AnnotateNode(const AnnotateNodeSignal& annotateNode)
        {
            if (m_remoteTools)
            {
                m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::AnnotateNode(annotateNode));
            }
        }

        void ServiceComponent::Visit(Message::AddBreakpointRequest&)
        {
            Lock lock(m_mutex);
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger has received an add breakpoint request!");
            // translate to runtime component values

            // !add the graph to the list of graphs being debugged!


            /*
            if (m_breakpoints.find(request.m_breakpoint) == m_breakpoints.end())
            {
                m_breakpoints.insert(request.m_breakpoint);
                remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, Message::BreakpointAdded(request.m_breakpoint));
            }
            */
        }

        void ServiceComponent::Visit(Message::AddTargetsRequest& request)
        {
            Lock lock(m_mutex);

            if (m_state == SCDebugState_Attached)
            {
                m_activeEntityStatusDirty = true;
                m_activeGraphStatusDirty = true;

                m_self.m_script.Merge(request.m_addTargets);
                m_client.m_script.Merge(request.m_addTargets);

                SetTargetsObserved(request.m_addTargets.m_entities, true);
                SetTargetsObserved(request.m_addTargets.m_staticEntities, true);
            }
        }

        void ServiceComponent::Visit(Message::RemoveTargetsRequest& request)
        {
            Lock lock(m_mutex);

            if (m_state == SCDebugState_Attached)
            {
                m_activeEntityStatusDirty = true;
                m_activeGraphStatusDirty = true;

                m_self.m_script.Remove(request.m_removeTargets);
                m_client.m_script.Remove(request.m_removeTargets);

                SetTargetsObserved(request.m_removeTargets.m_entities, false);
                SetTargetsObserved(request.m_removeTargets.m_staticEntities, false);
            }
        }

        void ServiceComponent::Visit(Message::StartLoggingRequest& request)
        {
            Lock lock(m_mutex);

            if (IsAttached())
            {
                m_self.m_script = request.m_initialTargets;
                m_self.m_script.m_logExecution = true;

                m_client.m_script = request.m_initialTargets;
                m_client.m_script.m_logExecution = true;

                SetTargetsObserved(request.m_initialTargets.m_entities, true);
                SetTargetsObserved(request.m_initialTargets.m_staticEntities, true);


                m_contextId = AzFramework::EntityContextId::CreateNull();

                if (!request.m_initialTargets.m_staticEntities.empty())
                {
                    AzFramework::GameEntityContextRequestBus::BroadcastResult(m_contextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
                }
            }
        }

        void ServiceComponent::Visit(Message::StopLoggingRequest& /*request*/)
        {
            Lock lock(m_mutex);

            if (IsAttached())
            {
                m_self.m_script.m_logExecution = false;
                m_client.m_script.m_logExecution = false;
            }
        }

        void ServiceComponent::Visit(Message::BreakRequest&)
        {
            if (m_state == SCDebugState_Attached)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is GOING TO BREAK!");
                m_state = SCDebugState_Interactive;
            }
            else
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is rejecting break request as it is not attached");
            }
        }

        void ServiceComponent::Visit(Message::ContinueRequest&)
        {
            if (m_state == SCDebugState_Interactive)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is CONTINUING TO RUN!");
                m_state = SCDebugState_Attached;
                if (m_remoteTools)
                {
                    m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::Continued());
                }
            }
            else
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is rejecting continue request as it is not interactive");
            }
        }

        void ServiceComponent::Visit(Message::GetAvailableScriptTargets&)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("received Message::GetAvailableScriptTargets");

            if (IsAttached())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("sending Message::GetAvailableScriptTargets");
                RefreshActiveEntityStatus();
                RefreshActiveGraphStatus();

                if (m_remoteTools)
                {
                    m_remoteTools->SendRemoteToolsMessage(
                        m_client.m_info, Message::AvailableScriptTargetsResult(AZStd::make_pair(m_activeEntities, m_activeGraphs)));
                }
            }
        }

        void ServiceComponent::Visit(Message::GetActiveEntitiesRequest&)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("received Message::GetActiveEntitiesRequest");

            if (IsAttached())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("sending Message::GetActiveEntitiesResult: %d", m_activeEntities.size());

                RefreshActiveEntityStatus();
                if (m_remoteTools)
                {
                    m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::ActiveEntitiesResult(m_activeEntities));
                }
            }
        }

        void ServiceComponent::Visit(Message::GetActiveGraphsRequest&)
        {
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("received Message::GetActiveGraphsRequest");

            if (IsAttached())
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("sending Message::GetActiveGraphsResult: %d", m_activeGraphs.size());

                RefreshActiveGraphStatus();
                if (m_remoteTools)
                {
                    m_remoteTools->SendRemoteToolsMessage(m_client.m_info, Message::ActiveGraphsResult(m_activeGraphs));
                }
            }
        }

        void ServiceComponent::Visit(Message::RemoveBreakpointRequest&)
        {
            Lock lock(m_mutex);
            SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger has received a remoe breakpoint request!");
            // translate to runtime component values

            // !remove the graph from the list of graphs being debugged!

            /*
            if (m_breakpoints.find(request.m_breakpoint) == m_breakpoints.end())
            {
            m_breakpoints.insert(request.m_breakpoint);
            remoteToolsInterface->SendRemoteToolsMessage(m_client.m_info, Message::BreakpointAdded(request.m_breakpoint));
            }
            */
        }

        void ServiceComponent::Visit(Message::StepOverRequest&)
        {
            if (m_state == SCDebugState_Interactive)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is going to step over the current instruction!");
                m_state = SCDebugState_InteractOnNext;
            }
            else
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_SERVER("The debugger is rejecting step over request as it is not interactive");
            }
        }

        void ServiceComponent::RuntimeError([[maybe_unused]] const ExecutionState& executionState, [[maybe_unused]] const AZStd::string_view& description)
        {
        }

        void ServiceComponent::SetTargetsObserved(const TargetEntities& targetEntities, [[maybe_unused]] bool observedState)
        {
             for (auto target : targetEntities)
             {
                 AZ::Entity* entity = nullptr;
                 AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, target.first);
 
                 if (entity)
                 {
                     auto runtimeComponents = AZ::EntityUtils::FindDerivedComponents<RuntimeComponent>(entity);
 
                     if (runtimeComponents.empty())
                     {
                         continue;
                     }
 
                     for (auto graphIdentifier : target.second)
                     {
                         for (auto graphIter = runtimeComponents.begin(); graphIter != runtimeComponents.end(); ++graphIter)
                         {
                             auto runtimeComponent = (*graphIter);
 
                             if (graphIdentifier.m_assetId.m_guid == runtimeComponent->GetRuntimeDataOverrides().m_runtimeAsset.GetId().m_guid)
                             {
                                 // TODO: Gate on ComponentId
                                 // runtimeComponent->SetIsGraphObserved(observedState);
                                 runtimeComponents.erase(graphIter);
                                 break;
                             }
                         }
                     }
                 }
             }
        }

        void ServiceComponent::DisconnectFromClient()
        {
            m_state = SCDebugState_Detaching;

            AzFramework::RemoteToolsEndpointInfo targetInfo = m_client.m_info;

            m_client = Target();
            m_self.m_script = ScriptTarget();

            if (m_remoteTools)
            {
                m_remoteTools->SendRemoteToolsMessage(targetInfo, Message::Disconnected());
            }

            m_state = SCDebugState_Detached;
        }

        void ServiceComponent::RefreshActiveEntityStatus()
        {
//             if (m_activeEntityStatusDirty)
//             {
//                 m_activeEntityStatusDirty = false;
// 
//                 for (auto& entityPair : m_activeEntities)
//                 {
//                      for (auto& graphPair : entityPair.second.m_activeGraphs)
//                      {
//                          graphPair.second.m_isObserved = IsGraphObserved(entityPair.first, graphPair.first);
//                      }
//                 }
//             }
        }

        void ServiceComponent::RefreshActiveGraphStatus()
        {
            if (m_activeGraphStatusDirty)
            {
                m_activeGraphStatusDirty = false;

                for (auto& graphPair : m_activeGraphs)
                {
                    graphPair.second.m_isObserved = IsAssetObserved(graphPair.first);
                }
            }
        }
    }
}
