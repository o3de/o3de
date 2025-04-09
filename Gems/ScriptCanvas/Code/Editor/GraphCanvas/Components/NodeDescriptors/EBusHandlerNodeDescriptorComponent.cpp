/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <qstring.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerNodeDescriptorComponent.h>

#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/GraphCanvas/DynamicSlotBus.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/Nodes/NodeDisplayUtils.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>

#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // EBusHandlerNodeDescriptorSaveData
    //////////////////////////////////////
    bool EBusHandlerNodeDescriptorSaveDataVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 2)
        {
            AZStd::vector< AZStd::string > eventNames;
            auto subElement = classElement.FindSubElement(AZ_CRC_CE("EventNames"));

            if (subElement)
            {
                if (!subElement->GetData(eventNames))
                {
                    return false;
                }
            }

            AZStd::vector< ScriptCanvas::EBusEventId > handlerEventIds;
            handlerEventIds.reserve(eventNames.size());

            for (const AZStd::string& eventName : eventNames)
            {
                handlerEventIds.emplace_back(eventName.c_str());
            }

            classElement.RemoveElementByName(AZ_CRC_CE("EventNames"));
            classElement.AddElementWithData(context, "EventIds", handlerEventIds);
        }

        return true;
    }

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::EBusHandlerNodeDescriptorSaveData()
        : m_displayConnections(false)
        , m_callback(nullptr)
    {
    }

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::EBusHandlerNodeDescriptorSaveData(EBusHandlerNodeDescriptorComponent* component)
        : m_displayConnections(false)
        , m_callback(component)
    {
    }

    void EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::operator=(const EBusHandlerNodeDescriptorSaveData& other)
    {
        // Purposefully skipping over the callback
        m_displayConnections = other.m_displayConnections;
        m_enabledEvents = other.m_enabledEvents;
    }

    void EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::OnDisplayConnectionsChanged()
    {
        if (m_callback)
        {
            m_callback->OnDisplayConnectionsChanged();
            SignalDirty();
        }
    }

    ///////////////////////////////////////
    // EBusHandlerNodeDescriptorComponent
    ///////////////////////////////////////

    bool EBusHandlerDescriptorVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 1)
        {
            AZ::Crc32 displayConnectionId = AZ_CRC_CE("DisplayConnections");

            EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(displayConnectionId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_displayConnections);
            }

            classElement.RemoveElementByName(displayConnectionId);
            classElement.RemoveElementByName(AZ_CRC_CE("BusName"));
            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }

    void EBusHandlerNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EBusHandlerNodeDescriptorSaveData, GraphCanvas::ComponentSaveData>()
                ->Version(2, &EBusHandlerNodeDescriptorSaveDataVersionConverter)
                ->Field("DisplayConnections", &EBusHandlerNodeDescriptorSaveData::m_displayConnections)
                ->Field("EventIds", &EBusHandlerNodeDescriptorSaveData::m_enabledEvents)
            ;
            
            serializeContext->Class<EBusHandlerNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(3, &EBusHandlerDescriptorVersionConverter)
                ->Field("SaveData", &EBusHandlerNodeDescriptorComponent::m_saveData)
                ->Field("BusName", &EBusHandlerNodeDescriptorComponent::m_busName)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EBusHandlerNodeDescriptorSaveData>("SaveData", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EBusHandlerNodeDescriptorSaveData::m_displayConnections, "Display Connection Controls", "Controls whether or not manual connection controls are visible for this node.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EBusHandlerNodeDescriptorSaveData::OnDisplayConnectionsChanged)
                    ;

                editContext->Class<EBusHandlerNodeDescriptorComponent>("Event Handler", "Configuration values for the EBus node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EBusHandlerNodeDescriptorComponent::m_saveData, "SaveData", "The modifiable information about this comment.")
                ;
            }
        }
    }

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_saveData(this)
        , m_loadingEvents(false)
    {
    }
    
    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorComponent(const AZStd::string& busName)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_saveData(this)
        , m_busName(busName)
        , m_loadingEvents(false)
    {
    }

    void EBusHandlerNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        EBusHandlerNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::GraphCanvasPropertyBusHandler::OnActivate(GetEntityId());
        GraphCanvas::WrapperNodeConfigurationRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EBusHandlerNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
        GraphCanvas::EntitySaveDataRequestBus::Handler::BusDisconnect();
        GraphCanvas::WrapperNodeConfigurationRequestBus::Handler::BusDisconnect();
        GraphCanvas::GraphCanvasPropertyBusHandler::OnDeactivate();
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusDisconnect();
        EBusHandlerNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    void EBusHandlerNodeDescriptorComponent::OnNodeActivated()
    {
        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetWrapperType, AZ::Crc32(m_busName.c_str()));
    }

    void EBusHandlerNodeDescriptorComponent::OnMemberSetupComplete()
    {
        m_loadingEvents = true;
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        AZStd::vector< HandlerEventConfiguration > eventConfigurations = GetEventConfigurations();

        for (const ScriptCanvas::EBusEventId& eventId : m_saveData.m_enabledEvents)
        {
            if (m_eventTypeToId.find(eventId) == m_eventTypeToId.end())
            {
                AZStd::string eventName;

                for (const HandlerEventConfiguration& testEventConfiguration : eventConfigurations)
                {
                    if (testEventConfiguration.m_eventId == eventId)
                    {
                        eventName = testEventConfiguration.m_eventName;
                    }
                }
    
                AZ::EntityId internalNode = Nodes::DisplayEbusEventNode(graphCanvasGraphId, m_busName, eventName, eventId);

                if (internalNode.IsValid())
                {
                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Add, internalNode, false);

                    GraphCanvas::WrappedNodeConfiguration configuration = GetEventConfiguration(eventId);
                    GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::WrapNode, internalNode, configuration);
                }
            }
        }

        m_loadingEvents = false;

        m_saveData.RegisterIds(GetEntityId(), graphCanvasGraphId);
    }

    void EBusHandlerNodeDescriptorComponent::OnSceneMemberDeserialized(const AZ::EntityId&, const GraphCanvas::GraphSerialization&)
    {
        m_saveData.m_enabledEvents.clear();
    }

    void EBusHandlerNodeDescriptorComponent::WriteSaveData(GraphCanvas::EntitySaveDataContainer& saveDataContainer) const
    {
        EBusHandlerNodeDescriptorSaveData* saveData = saveDataContainer.FindCreateSaveData<EBusHandlerNodeDescriptorSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void EBusHandlerNodeDescriptorComponent::ReadSaveData(const GraphCanvas::EntitySaveDataContainer& saveDataContainer)
    {
        const EBusHandlerNodeDescriptorSaveData* saveData = saveDataContainer.FindSaveDataAs<EBusHandlerNodeDescriptorSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    AZStd::string_view EBusHandlerNodeDescriptorComponent::GetBusName() const
    {
        return m_busName;
    }

    GraphCanvas::WrappedNodeConfiguration EBusHandlerNodeDescriptorComponent::GetEventConfiguration(const ScriptCanvas::EBusEventId& eventId) const
    {
        AZ_Warning("ScriptCanvas", m_scriptCanvasId.IsValid(), "Trying to query event list before the node is added to the scene.");

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

        GraphCanvas::WrappedNodeConfiguration wrappedConfiguration;

        if (entity)
        {
            ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

            if (eventHandler)
            {
                const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = eventHandler->GetEvents();

                AZ::u32 i = 0;
                for (const auto& event : events)
                {
                    if (event.second.m_eventId == eventId)
                    {
                        wrappedConfiguration.m_layoutOrder = i;
                        break;
                    }

                    ++i;
                }
            }
        }

        return wrappedConfiguration;
    }

    bool EBusHandlerNodeDescriptorComponent::ContainsEvent(const ScriptCanvas::EBusEventId& eventId) const
    {
        return m_eventTypeToId.find(eventId) != m_eventTypeToId.end();
    }

    AZStd::vector< HandlerEventConfiguration > EBusHandlerNodeDescriptorComponent::GetEventConfigurations() const
    {
        AZ_Warning("ScriptCanvas", m_scriptCanvasId.IsValid(), "Trying to query event list before the node is added to the scene.");
        AZStd::vector< HandlerEventConfiguration > eventConfigurations;

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

        if (entity)
        {
            ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

            if (eventHandler)
            {
                const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = eventHandler->GetEvents();

                eventConfigurations.reserve(events.size());

                for (const auto& eventEntry : events)
                {
                    HandlerEventConfiguration configuration;
                    configuration.m_eventName = eventEntry.second.m_eventName;
                    configuration.m_eventId = eventEntry.second.m_eventId;

                    eventConfigurations.emplace_back(configuration);
                }
            }
        }

        return eventConfigurations;
    }

    AZ::EntityId EBusHandlerNodeDescriptorComponent::FindEventNodeId(const ScriptCanvas::EBusEventId& eventId) const
    {
        AZ::EntityId retVal;

        auto iter = m_eventTypeToId.find(eventId);

        if (iter != m_eventTypeToId.end())
        {
            retVal = iter->second;
        }

        return retVal;
    }

    AZ::EntityId EBusHandlerNodeDescriptorComponent::FindGraphCanvasNodeIdForSlot(const ScriptCanvas::SlotId& slotId) const
    {
        ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(m_scriptCanvasId);

        if (eventHandler)
        {
            auto nonEventSlotIds = eventHandler->GetNonEventSlotIds();

            bool isNonEventSlot = (AZStd::find(nonEventSlotIds.begin(), nonEventSlotIds.end(), slotId) != nonEventSlotIds.end());

            if (isNonEventSlot)
            {
                return GetEntityId();
            }

            auto scriptEvents = eventHandler->GetEvents();

            ScriptCanvas::EBusEventId foundEventId = ScriptCanvas::EBusEventId();

            for (auto scriptEventPair : scriptEvents)
            {
                const auto& scriptEvent = scriptEventPair.second;

                bool foundResult = scriptEvent.m_eventSlotId == slotId;
                foundResult = foundResult || scriptEvent.m_resultSlotId == slotId;
                foundResult = foundResult || (AZStd::find(scriptEvent.m_parameterSlotIds.begin(), scriptEvent.m_parameterSlotIds.end(), slotId) != nonEventSlotIds.end());

                if (foundResult)
                {
                    foundEventId = scriptEventPair.first;
                    break;
                }
            }

            if (foundEventId != ScriptCanvas::EBusEventId())
            {
                return FindEventNodeId(foundEventId);
            }
        }

        return AZ::EntityId();
    }

    GraphCanvas::Endpoint EBusHandlerNodeDescriptorComponent::MapSlotToGraphCanvasEndpoint(const ScriptCanvas::SlotId& scriptCanvasSlotId) const
    {
        GraphCanvas::Endpoint endpoint;

        AZ::EntityId graphCanvasSlotId;
        SlotMappingRequestBus::EventResult(graphCanvasSlotId, GetEntityId(), &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSlotId);

        if (!graphCanvasSlotId.IsValid())
        {
            for (auto& mapPair : m_eventTypeToId)
            {
                SlotMappingRequestBus::EventResult(graphCanvasSlotId, mapPair.second, &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSlotId);

                if (graphCanvasSlotId.IsValid())
                {
                    endpoint = GraphCanvas::Endpoint(mapPair.second, graphCanvasSlotId);
                    break;
                }
            }
        }
        else
        {
            endpoint = GraphCanvas::Endpoint(GetEntityId(), graphCanvasSlotId);
        }

        return endpoint;
    }

    void EBusHandlerNodeDescriptorComponent::OnWrappedNode(const AZ::EntityId& wrappedNode)
    {
        ScriptCanvas::EBusEventId eventId;
        EBusHandlerEventNodeDescriptorRequestBus::EventResult(eventId, wrappedNode, &EBusHandlerEventNodeDescriptorRequests::GetEventId);

        if (eventId == ScriptCanvas::EBusEventId())
        {
            AZ_Warning("ScriptCanvas", false, "Trying to wrap an event node without an event name being specified.");
            return;
        }

        auto emplaceResult = m_eventTypeToId.emplace(eventId, wrappedNode);

        if (emplaceResult.second)
        {
            m_idToEventType.emplace(wrappedNode, eventId);

            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, wrappedNode, &GraphCanvas::NodeRequests::GetUserData);

            if (userData)
            {
                (*userData) = m_scriptCanvasId;
                DynamicSlotRequestBus::Event(wrappedNode, &DynamicSlotRequests::OnUserDataChanged);

                GraphCanvas::NodeDataSlotRequestBus::Event(wrappedNode, &GraphCanvas::NodeDataSlotRequests::RecreatePropertyDisplay);
            }
            
            if (!m_loadingEvents)
            {
                m_saveData.m_enabledEvents.emplace_back(eventId);
                m_saveData.SignalDirty();
            }
        }
        // If we are wrapping the same node twice for just ignore it and log a message
        else if (emplaceResult.first->second != wrappedNode)
        {
            AZ_Error("ScriptCanvas", false, "Trying to wrap two identically named methods under the same EBus Handler. Deleting the second node.");

            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            AZStd::unordered_set<AZ::EntityId> deleteNodes;
            deleteNodes.insert(wrappedNode);
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, deleteNodes);
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "Trying to wrap the same node twice.");
        }
    }

    void EBusHandlerNodeDescriptorComponent::OnUnwrappedNode(const AZ::EntityId& unwrappedNode)
    {
        auto iter = m_idToEventType.find(unwrappedNode);

        if (iter != m_idToEventType.end())
        {
            ScriptCanvas::EBusEventId eventId = iter->second;

            m_eventTypeToId.erase(eventId);
            m_idToEventType.erase(iter);

            for (auto eventIter = m_saveData.m_enabledEvents.begin(); eventIter != m_saveData.m_enabledEvents.end(); ++eventIter)
            {
                if ((*eventIter) == eventId)
                {
                    m_saveData.m_enabledEvents.erase(eventIter);
                    m_saveData.SignalDirty();
                    break;
                }
            }
        }
    }

    GraphCanvas::WrappedNodeConfiguration EBusHandlerNodeDescriptorComponent::GetWrappedNodeConfiguration(const AZ::EntityId& wrappedNodeId) const
    {
        ScriptCanvas::EBusEventId eventId;
        EBusHandlerEventNodeDescriptorRequestBus::EventResult(eventId, wrappedNodeId, &EBusHandlerEventNodeDescriptorRequests::GetEventId);

        return GetEventConfiguration(eventId);
    }

    AZ::Component* EBusHandlerNodeDescriptorComponent::GetPropertyComponent()
    {
        return this;
    }

    void EBusHandlerNodeDescriptorComponent::OnAddedToGraphCanvasGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        m_scriptCanvasId = scriptCanvasNodeId;

        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetActionString, "Add/Remove Events");
        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_saveData.m_displayConnections);

        if (m_scriptCanvasId.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_scriptCanvasId);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

                if (eventHandler && eventHandler->IsIDRequired())
                {
                    AZStd::vector< AZ::EntityId > slotIds;
                    GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                    // Should be exactly one data slot on ourselves. And that is the BusId
                    for (const AZ::EntityId& testSlotId : slotIds)
                    {
                        GraphCanvas::SlotType slotType;
                        GraphCanvas::SlotRequestBus::EventResult(slotType, testSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                        if (slotType == GraphCanvas::SlotTypes::DataSlot)
                        {
                            GraphCanvas::TranslationKey key;
                            key << ScriptCanvasEditor::TranslationHelper::GlobalKeys::EBusHandlerIDKey << ".details";
                            GraphCanvas::TranslationRequests::Details details;
                            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);
                            GraphCanvas::SlotRequestBus::Event(testSlotId, &GraphCanvas::SlotRequests::SetDetails, details.m_name, details.m_tooltip);
                            break;
                        }
                    }
                }
            }
        }
    }

    void EBusHandlerNodeDescriptorComponent::OnDisplayConnectionsChanged()
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

        if (entity)
        {
            ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

            if (eventHandler)
            {
                // If we are hiding the connections, we need to confirm that
                // everything will be ok(i.e. no active connections)
                if (!m_saveData.m_displayConnections)
                {
                    AZStd::vector< ScriptCanvas::SlotId > scriptCanvasSlots = eventHandler->GetNonEventSlotIds();

                    bool allowChange = true;

                    for (const auto& slotId : scriptCanvasSlots)
                    {
                        ScriptCanvas::Slot* slot = eventHandler->GetSlot(slotId);

                        if (slot->IsExecution() && eventHandler->IsConnected((*slot)))
                        {
                            allowChange = false;
                            break;
                        }
                    }

                    if (!allowChange)
                    {
                        AZ_Warning("Script Canvas", false, "Cannot hide EBus Connection Controls because one ore more slots are currently connected. Please disconnect all slots to hide.");
                        m_saveData.m_displayConnections = true;
                        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                    }
                }

                eventHandler->SetAutoConnectToGraphOwner(!m_saveData.m_displayConnections);
            }
        }

        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_saveData.m_displayConnections);
    }
}
