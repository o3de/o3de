/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <Editor/Nodes/NodeDisplayUtils.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverEventNodeDescriptorComponent.h>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>

#include <Libraries/Core/ReceiveScriptEvent.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////////////////////////
    // ScriptEventReceiverEventNodeDescriptorComponent
    ////////////////////////////////////////////////////

    void ScriptEventReceiverEventNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ScriptEventReceiverEventNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(3)
                ->Field("AssetId", &ScriptEventReceiverEventNodeDescriptorComponent::m_assetId)
                ->Field("BusName", &ScriptEventReceiverEventNodeDescriptorComponent::m_busName)
                ->Field("MethodDefinition", &ScriptEventReceiverEventNodeDescriptorComponent::m_methodDefinition)
                ->Field("EventName", &ScriptEventReceiverEventNodeDescriptorComponent::m_eventName)
                ->Field("EventId", &ScriptEventReceiverEventNodeDescriptorComponent::m_eventId)
                ->Field("EventPropertyId", &ScriptEventReceiverEventNodeDescriptorComponent::m_eventPropertyId)
                ;
        }
    }

    ScriptEventReceiverEventNodeDescriptorComponent::ScriptEventReceiverEventNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandlerEvent)
    {
    }

    ScriptEventReceiverEventNodeDescriptorComponent::ScriptEventReceiverEventNodeDescriptorComponent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandlerEvent)
        , m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
    {
        m_eventName = m_methodDefinition.GetName();
        m_eventId = m_methodDefinition.GetEventId();
        m_eventPropertyId = m_methodDefinition.GetNameProperty().GetId();
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        ScriptEventReceiverEventNodeDescriptorBus::Handler::BusConnect(GetEntityId());
        EBusHandlerEventNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::ForcedWrappedNodeRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());

        m_busId = AZ::Crc32(m_assetId.ToString<AZStd::string>().c_str());
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
        GraphCanvas::ForcedWrappedNodeRequestBus::Handler::BusDisconnect();
        EBusHandlerEventNodeDescriptorRequestBus::Handler::BusDisconnect();
        ScriptEventReceiverEventNodeDescriptorBus::Handler::BusDisconnect();
    }

    AZStd::string_view ScriptEventReceiverEventNodeDescriptorComponent::GetBusName() const
    {
        return m_busName;
    }

    AZStd::string_view ScriptEventReceiverEventNodeDescriptorComponent::GetEventName() const
    {
        return m_eventName;
    }

    ScriptCanvas::EBusEventId ScriptEventReceiverEventNodeDescriptorComponent::GetEventId() const
    {
        return m_eventId;
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::SetHandlerAddress(const ScriptCanvas::Datum& idDatum)
    {
        if (m_ebusWrapper.m_scriptCanvasId.IsValid())
        {
            GraphCanvas::GraphId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            ScriptCanvas::ScriptCanvasId canvasId;
            GeneralRequestBus::BroadcastResult(canvasId, &GeneralRequests::GetScriptCanvasId, graphId);

            ScriptCanvas::GraphScopedNodeId scopedNodeId(canvasId, m_ebusWrapper.m_scriptCanvasId);
            ScriptCanvas::EBusHandlerNodeRequestBus::Event(scopedNodeId, &ScriptCanvas::EBusHandlerNodeRequests::SetAddressId, AZStd::move(idDatum));

            m_queuedId = AZStd::move(ScriptCanvas::Datum());
        }
        else
        {
            m_queuedId.DeepCopyDatum(idDatum);
        }
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::OnNodeWrapped(const AZ::EntityId& wrappingNode)
    {
        bool isValid = false;

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, wrappingNode, &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            AZ_Warning("ScriptCanvas", !m_ebusWrapper.m_graphCanvasId.IsValid() && !m_ebusWrapper.m_scriptCanvasId.IsValid(), "Wrapping the same ebus event node twice without unwrapping it.");
            AZ::EntityId scriptCanvasId = (*AZStd::any_cast<AZ::EntityId>(userData));

            m_ebusWrapper.m_graphCanvasId = wrappingNode;
            m_ebusWrapper.m_scriptCanvasId = scriptCanvasId;

            ScriptCanvas::Nodes::Core::ReceiveScriptEvent* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(scriptCanvasId);
            if (eventHandler)
            {
                AZStd::vector< AZ::EntityId > graphCanvasSlotIds;
                GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                const ScriptCanvas::Nodes::Core::SendScriptEvent::EventMap& events = eventHandler->GetEvents();
                ScriptCanvas::Nodes::Core::Internal::ScriptEventEntry myEvent;

                auto mapIter = events.find(m_eventId);

                if (mapIter != events.end())
                {
                    myEvent = mapIter->second;
                }

                const int numEventSlots = 1;
                const int numResultSlots = myEvent.m_resultSlotId.IsValid() ? 1 : 0;

                const int totalSlots = (myEvent.m_numExpectedArguments + numEventSlots + numResultSlots);

                // Potentially overly simplistic way of detecting if we need to refresh our Slot information.
                if (totalSlots != graphCanvasSlotIds.size())
                {
                    // Remove the previous slots
                    for (const auto& graphCanvasSlotId : graphCanvasSlotIds)
                    {
                        GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::RemoveSlot, graphCanvasSlotId);
                    }

                    // Then from a clean slate, fully regenerate all of our slots.
                    ScriptCanvas::Slot* scriptCanvasSlot = eventHandler->GetSlot(myEvent.m_eventSlotId);

                    if (scriptCanvasSlot && scriptCanvasSlot->IsVisible())
                    {
                        Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*scriptCanvasSlot), 0);
                    }

                    int paramIndex = 0;
                    int outputIndex = 0;
                    //
                    // inputCount and outputCount work because the order of the slots is maintained from the BehaviorContext, if this changes
                    // in the future then we should consider storing the actual offset or key name at that time.
                    //
                    for (const auto& slotId : myEvent.m_parameterSlotIds)
                    {
                        scriptCanvasSlot = eventHandler->GetSlot(slotId);

                        int& index = (scriptCanvasSlot->IsData() && scriptCanvasSlot->IsInput()) ? paramIndex : outputIndex;

                        if (scriptCanvasSlot && scriptCanvasSlot->IsVisible())
                        {
                            Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*scriptCanvasSlot), index);
                        }

                        ++index;
                    }

                    if (myEvent.m_resultSlotId.IsValid())
                    {
                        scriptCanvasSlot = eventHandler->GetSlot(myEvent.m_resultSlotId);

                        if (scriptCanvasSlot && scriptCanvasSlot->IsVisible())
                        {
                            Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*scriptCanvasSlot), 0);
                        }
                    }

                    GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);
                }

                isValid = (totalSlots == graphCanvasSlotIds.size());
            }

            if (m_queuedId.GetType().IsValid())
            {
                SetHandlerAddress(m_queuedId);
            }
        }

        if (!isValid)
        {
            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            if (sceneId.IsValid())
            {
                AZStd::unordered_set<AZ::EntityId> deleteNodes;
                deleteNodes.insert(GetEntityId());

                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, deleteNodes);
            }
        }
        else
        {
            ScriptEventReceiveNodeDescriptorNotificationBus::Handler::BusConnect(wrappingNode);
        }
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::OnSceneMemberAboutToSerialize(GraphCanvas::GraphSerialization& sceneSerialization)
    {
        AZ::Entity* wrapperEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(wrapperEntity, &AZ::ComponentApplicationRequests::FindEntity, m_ebusWrapper.m_graphCanvasId);

        if (wrapperEntity)
        {
            sceneSerialization.GetGraphData().m_nodes.insert(wrapperEntity);
        }
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::OnSceneMemberDeserialized([[maybe_unused]] const AZ::EntityId& graphId, [[maybe_unused]] const GraphCanvas::GraphSerialization& serializationTarget)
    {
        // Add in the asset bus hook up here if we want to ensure we get asset notifications post copy/paste
    }

    AZ::Crc32 ScriptEventReceiverEventNodeDescriptorComponent::GetWrapperType() const
    {
        return m_busId;
    }

    AZ::Crc32 ScriptEventReceiverEventNodeDescriptorComponent::GetIdentifier() const
    {
        return AZ::Crc32(m_eventName.c_str());
    }

    AZ::EntityId ScriptEventReceiverEventNodeDescriptorComponent::CreateWrapperNode(const AZ::EntityId& sceneId, const AZ::Vector2& nodePosition)
    {
        CreateScriptEventsHandlerMimeEvent createEbusMimeEvent(m_assetId, m_methodDefinition);

        AZ::EntityId wrapperNode;
        AZ::Vector2 dummyPosition = nodePosition;

        if (createEbusMimeEvent.ExecuteEvent(nodePosition, dummyPosition, sceneId))
        {
            const NodeIdPair& nodePair = createEbusMimeEvent.GetCreatedPair();

            wrapperNode = nodePair.m_graphCanvasId;
        }

        return wrapperNode;
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::OnScriptEventReloaded(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>& asset)
    {
        bool isValid = false;
        ScriptEvents::ScriptEvent definition = asset.Get()->m_definition;

        for (auto methodDefinition : definition.GetMethods())
        {
            if (methodDefinition.GetEventId() == m_eventId)
            {
                m_methodDefinition = methodDefinition;
                isValid = true;
                break;
            }
        }

        if (isValid)
        {
            UpdateTitles();
        }
        else
        {
            AZ::EntityId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            // From this point on this element is garbage, so do not reference anything internal
            AZStd::unordered_set< AZ::EntityId > deleteSet = { GetEntityId() };
            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deleteSet);
        }
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::OnAddedToGraphCanvasGraph([[maybe_unused]] const GraphCanvas::GraphId& grapphId, [[maybe_unused]] const AZ::EntityId& scriptCanvasNodeId)
    {
        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

        for (const auto& slotId : slotIds)
        {
            GraphCanvas::SlotType slotType;
            GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_None;
                GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

                if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                {
                    break;
                }
            }
        }

        UpdateTitles();
    }

    void ScriptEventReceiverEventNodeDescriptorComponent::UpdateTitles()
    {
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, m_methodDefinition.GetName());
        GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, m_methodDefinition.GetTooltip());
    }
}
