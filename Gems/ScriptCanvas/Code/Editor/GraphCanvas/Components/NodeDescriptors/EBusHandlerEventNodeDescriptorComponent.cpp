/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GeometryBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerEventNodeDescriptorComponent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/EBusNodeBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <Editor/Nodes/NodeDisplayUtils.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/Translation/TranslationHelper.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////////////////
    // EBusHandlerEventNodeDescriptorComponent
    ////////////////////////////////////////////

    static bool EBusHandlerEventNodeDescriptorComponentVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() <= 1)
        {
            auto element = rootElement.FindSubElement(AZ_CRC("EventName", 0x0287a22f));

            AZStd::string eventName;
            if (element->GetData(eventName))
            {
                AZ::Crc32 eventId = AZ::Crc32(eventName.c_str());

                if (rootElement.AddElementWithData(serializeContext, "EventId", eventId) == -1)
                {
                    return false;
                }
            }
        }

        return true;
    }
    
    void EBusHandlerEventNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EBusHandlerEventNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(2, EBusHandlerEventNodeDescriptorComponentVersionConverter)
                ->Field("BusName", &EBusHandlerEventNodeDescriptorComponent::m_busName)
                ->Field("EventName", &EBusHandlerEventNodeDescriptorComponent::m_eventName)
                ->Field("EventId", &EBusHandlerEventNodeDescriptorComponent::m_eventId)
            ;
        }
    }

    EBusHandlerEventNodeDescriptorComponent::EBusHandlerEventNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandlerEvent)
    {
    }
    
    EBusHandlerEventNodeDescriptorComponent::EBusHandlerEventNodeDescriptorComponent(const AZStd::string& busName, const AZStd::string& eventName, ScriptCanvas::EBusEventId eventId)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandlerEvent)
        , m_busName(busName)
        , m_eventName(eventName)
        , m_eventId(eventId)
    {
    }

    void EBusHandlerEventNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        EBusHandlerEventNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());        
        GraphCanvas::ForcedWrappedNodeRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EBusHandlerEventNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
        GraphCanvas::ForcedWrappedNodeRequestBus::Handler::BusDisconnect();
        EBusHandlerEventNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    AZStd::string_view EBusHandlerEventNodeDescriptorComponent::GetBusName() const
    {
        return m_busName;
    }

    AZStd::string_view EBusHandlerEventNodeDescriptorComponent::GetEventName() const
    {
        return m_eventName;
    }

    ScriptCanvas::EBusEventId EBusHandlerEventNodeDescriptorComponent::GetEventId() const
    {
        return m_eventId;
    }

    void EBusHandlerEventNodeDescriptorComponent::SetHandlerAddress(const ScriptCanvas::Datum& idDatum)
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

    void EBusHandlerEventNodeDescriptorComponent::OnNodeWrapped(const AZ::EntityId& wrappingNode)
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

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, scriptCanvasId);

            AZStd::string ebusContextName = TranslationHelper::GetEbusHandlerContext(m_busName);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

                if (eventHandler)
                {
                    AZStd::vector< AZ::EntityId > graphCanvasSlotIds;
                    GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                    const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = eventHandler->GetEvents();
                    ScriptCanvas::Nodes::Core::EBusEventEntry myEvent;

                    for (const auto& event : events)
                    {
                        if (event.second.m_eventName.compare(m_eventName) == 0)
                        {
                            myEvent = event.second;
                            break;
                        }
                    }

                    const int numEventSlots = 1;
                    const int numResultSlots = myEvent.m_resultSlotId.IsValid() ?  1 : 0;

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
                            AZ::EntityId slotId = Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*scriptCanvasSlot));

                            GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerOnEventTriggeredNameKey());
                            GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerOnEventTriggeredTooltipKey());
                        }

                        //
                        // inputCount and outputCount work because the order of the slots is maintained from the BehaviorContext, if this changes
                        // in the future then we should consider storing the actual offset or key name at that time.
                        //
                        int inputCount = 0;
                        int outputCount = 0;
                        for (const auto& slotId : myEvent.m_parameterSlotIds)
                        {
                            scriptCanvasSlot = eventHandler->GetSlot(slotId);

                            if (scriptCanvasSlot && scriptCanvasSlot->IsVisible())
                            {
                                AZ::EntityId graphCanvasSlotId = Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*scriptCanvasSlot));

                                TranslationItemType itemType = TranslationHelper::GetItemType(scriptCanvasSlot->GetDescriptor());

                                GraphCanvas::TranslationKeyedString slotNameKeyedString(scriptCanvasSlot->GetName());
                                slotNameKeyedString.m_context = ebusContextName;

                                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(scriptCanvasSlot->GetToolTip());
                                slotTooltipKeyedString.m_context = ebusContextName;

                                slotNameKeyedString.SetFallback(scriptCanvasSlot->GetName());
                                slotTooltipKeyedString.SetFallback(scriptCanvasSlot->GetToolTip());

                                if (scriptCanvasSlot->GetDescriptor() == ScriptCanvas::SlotDescriptors::DataOut())
                                {
                                    slotNameKeyedString.m_key = TranslationHelper::GetEBusHandlerSlotKey(m_busName, m_eventName, itemType, TranslationKeyId::Name, outputCount);
                                    slotTooltipKeyedString.m_key = TranslationHelper::GetEBusHandlerSlotKey(m_busName, m_eventName, itemType, TranslationKeyId::Tooltip, outputCount);
                                    ++outputCount;
                                }
                                else
                                {
                                    slotNameKeyedString.m_key = TranslationHelper::GetEBusHandlerSlotKey(m_busName, m_eventName, itemType, TranslationKeyId::Name, inputCount);
                                    slotTooltipKeyedString.m_key = TranslationHelper::GetEBusHandlerSlotKey(m_busName, m_eventName, itemType, TranslationKeyId::Tooltip, inputCount);
                                    ++inputCount;
                                }

                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
                            }
                        }

                        if (myEvent.m_resultSlotId.IsValid())
                        {
                            scriptCanvasSlot = eventHandler->GetSlot(myEvent.m_resultSlotId);

                            if (scriptCanvasSlot && scriptCanvasSlot->IsVisible())
                            {
                                AZ::EntityId graphCanvasSlotId = Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*scriptCanvasSlot));

                                TranslationItemType itemType = TranslationHelper::GetItemType(scriptCanvasSlot->GetDescriptor());

                                GraphCanvas::TranslationKeyedString slotNameKeyedString(scriptCanvasSlot->GetName(), ebusContextName);
                                slotNameKeyedString.m_key = slotNameKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Name);

                                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(TranslationHelper::GetSafeTypeName(scriptCanvasSlot->GetDataType()), ebusContextName);
                                slotTooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Tooltip);

                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
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
        }

        if (!isValid)
        {
            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            if (sceneId.IsValid())
            {
                AZ_Error("GraphCanvas", false, "Failed to find valid configuration for EBusEventNode(%s::%s). Deleting Node", m_busName.c_str(), m_eventName.c_str());

                AZStd::unordered_set<AZ::EntityId> deleteNodes;
                deleteNodes.insert(GetEntityId());

                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, deleteNodes);
            }
        }
    }

    void EBusHandlerEventNodeDescriptorComponent::OnSceneMemberAboutToSerialize(GraphCanvas::GraphSerialization& sceneSerialization)
    {
        AZ::Entity* wrapperEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(wrapperEntity, &AZ::ComponentApplicationRequests::FindEntity, m_ebusWrapper.m_graphCanvasId);

        if (wrapperEntity)
        {
            sceneSerialization.GetGraphData().m_nodes.insert(wrapperEntity);
        }
    }

    AZ::Crc32 EBusHandlerEventNodeDescriptorComponent::GetWrapperType() const
    {
        return AZ::Crc32(m_busName.c_str());
    }

    AZ::Crc32 EBusHandlerEventNodeDescriptorComponent::GetIdentifier() const
    {
        return AZ::Crc32(m_eventName.c_str());
    }

    AZ::EntityId EBusHandlerEventNodeDescriptorComponent::CreateWrapperNode(const AZ::EntityId& sceneId, const AZ::Vector2& nodePosition)
    {
        CreateEBusHandlerMimeEvent createEbusMimeEvent(m_busName);

        AZ::EntityId wrapperNode;
        AZ::Vector2 dummyPosition = nodePosition;

        if (createEbusMimeEvent.ExecuteEvent(nodePosition, dummyPosition, sceneId))
        {
            const NodeIdPair& nodePair = createEbusMimeEvent.GetCreatedPair();

            wrapperNode = nodePair.m_graphCanvasId;
        }

        return wrapperNode;
    }

    void EBusHandlerEventNodeDescriptorComponent::OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId&, const AZ::EntityId&)
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
                    GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerOnEventTriggeredNameKey());
                    GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerOnEventTriggeredTooltipKey());

                    break;
                }
            }
        }
    }
}
