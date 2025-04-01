/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <QString>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/Nodes/NodeDisplayUtils.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/PropertyGridBus.h>

#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/GraphCanvas/DynamicSlotBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <AzCore/Math/Crc.h>
#include <ScriptCanvas/Libraries/Core/ScriptEventBase.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////////////////////////
    // ScriptEventReceiverHandlerNodeDescriptorSaveData
    /////////////////////////////////////////////////////

    ScriptEventReceiverNodeDescriptorComponent::ScriptEventReceiverHandlerNodeDescriptorSaveData::ScriptEventReceiverHandlerNodeDescriptorSaveData()
        : m_displayConnections(false)
        , m_callback(nullptr)
    {
    }

    ScriptEventReceiverNodeDescriptorComponent::ScriptEventReceiverHandlerNodeDescriptorSaveData::ScriptEventReceiverHandlerNodeDescriptorSaveData(ScriptEventReceiverNodeDescriptorComponent* component)
        : m_displayConnections(false)
        , m_callback(component)
    {
    }

    void ScriptEventReceiverNodeDescriptorComponent::ScriptEventReceiverHandlerNodeDescriptorSaveData::operator=(const ScriptEventReceiverHandlerNodeDescriptorSaveData& other)
    {
        // Purposefully skipping over the callback
        m_displayConnections = other.m_displayConnections;
        m_enabledEvents = other.m_enabledEvents;
    }

    void ScriptEventReceiverNodeDescriptorComponent::ScriptEventReceiverHandlerNodeDescriptorSaveData::OnDisplayConnectionsChanged()
    {
        if (m_callback)
        {
            m_callback->OnDisplayConnectionsChanged();
            SignalDirty();
        }
    }

    ///////////////////////////////////////////////
    // ScriptEventReceiverNodeDescriptorComponent
    ///////////////////////////////////////////////

    void ScriptEventReceiverNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ScriptEventReceiverHandlerNodeDescriptorSaveData, GraphCanvas::ComponentSaveData>()
                ->Version(2)
                ->Field("DisplayConnections", &ScriptEventReceiverHandlerNodeDescriptorSaveData::m_displayConnections)
                ->Field("EventNames", &ScriptEventReceiverHandlerNodeDescriptorSaveData::m_enabledEvents)
                ;

            serializeContext->Class<ScriptEventReceiverNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(3)
                ->Field("AssetId", &ScriptEventReceiverNodeDescriptorComponent::m_scriptEventsAssetId)
                ->Field("SaveData", &ScriptEventReceiverNodeDescriptorComponent::m_saveData)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<ScriptEventReceiverHandlerNodeDescriptorSaveData>("SaveData", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEventReceiverHandlerNodeDescriptorSaveData::m_displayConnections, "Display Connection Controls", "Controls whether or not manual connection controls are visible for this node.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptEventReceiverHandlerNodeDescriptorSaveData::OnDisplayConnectionsChanged)
                    ;

                editContext->Class<ScriptEventReceiverNodeDescriptorComponent>("Script Event Handler", "Configuration values for the Script Event Receiver node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEventReceiverNodeDescriptorComponent::m_saveData, "SaveData", "The modifiable information about this comment.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEventReceiverNodeDescriptorComponent::m_scriptEventsAssetId, "Asset Id", "The Script Event Asset Id to use.")
                    ;
            }
        }
    }

    ScriptEventReceiverNodeDescriptorComponent::ScriptEventReceiverNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_saveData(this)
        , m_loadingEvents(false)
    {
    }

    ScriptEventReceiverNodeDescriptorComponent::ScriptEventReceiverNodeDescriptorComponent(AZ::Data::AssetId assetId)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_saveData(this)
        , m_scriptEventsAssetId(assetId)
        , m_loadingEvents(false)
    {
    }

    void ScriptEventReceiverNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();        

        EBusHandlerNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());        
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::GraphCanvasPropertyBusHandler::OnActivate(GetEntityId());
        GraphCanvas::WrapperNodeConfigurationRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        ScriptEventReceiverNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());

        m_busId = AZ::Crc32(m_scriptEventsAssetId.ToString<AZStd::string>().c_str());
        AZ::Data::AssetBus::Handler::BusConnect(m_scriptEventsAssetId);
    }

    void ScriptEventReceiverNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        AZ::Data::AssetBus::Handler::BusDisconnect();

        ScriptEventReceiverNodeDescriptorRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
        GraphCanvas::EntitySaveDataRequestBus::Handler::BusDisconnect();
        GraphCanvas::WrapperNodeConfigurationRequestBus::Handler::BusDisconnect();
        GraphCanvas::GraphCanvasPropertyBusHandler::OnDeactivate();
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusDisconnect();        
        EBusHandlerNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnNodeActivated()
    {
        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetWrapperType, m_busId);
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnMemberSetupComplete()
    {
        m_loadingEvents = true;

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventsAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();

        if (asset.IsReady())
        {
            AZ::EntityId graphCanvasGraphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            ScriptEvents::ScriptEventsAsset* scriptEvent = asset.GetAs<ScriptEvents::ScriptEventsAsset>();

            m_busName = scriptEvent->m_definition.GetName();

            for (const auto& eventId : m_saveData.m_enabledEvents)
            {
                if (m_eventTypeToId.find(eventId.first) == m_eventTypeToId.end())
                {
                    AZ::EntityId internalNode;

                    for (auto& method : scriptEvent->m_definition.GetMethods())
                    {
                        if (eventId.first == method.GetEventId())
                        {
                            internalNode = Nodes::DisplayScriptEventNode(graphCanvasGraphId, m_scriptEventsAssetId, method);
                            break;
                        }
                    }

                    if (internalNode.IsValid())
                    {
                        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Add, internalNode, false);

                        GraphCanvas::WrappedNodeConfiguration configuration = GetEventConfiguration(eventId.first);
                        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::WrapNode, internalNode, configuration);
                    }
                }
            }
            m_loadingEvents = false;

            m_saveData.RegisterIds(GetEntityId(), graphCanvasGraphId);
        }
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnSceneMemberDeserialized(const AZ::EntityId&, const GraphCanvas::GraphSerialization&)
    {
        m_saveData.m_enabledEvents.clear();
        AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventsAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
    }

    void ScriptEventReceiverNodeDescriptorComponent::WriteSaveData(GraphCanvas::EntitySaveDataContainer& saveDataContainer) const
    {
        ScriptEventReceiverHandlerNodeDescriptorSaveData* saveData = saveDataContainer.FindCreateSaveData<ScriptEventReceiverHandlerNodeDescriptorSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void ScriptEventReceiverNodeDescriptorComponent::ReadSaveData(const GraphCanvas::EntitySaveDataContainer& saveDataContainer)
    {
        const ScriptEventReceiverHandlerNodeDescriptorSaveData* saveData = saveDataContainer.FindSaveDataAs<ScriptEventReceiverHandlerNodeDescriptorSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    AZ::Data::AssetId ScriptEventReceiverNodeDescriptorComponent::GetAssetId() const
    {
        return m_scriptEventsAssetId;
    }

    AZStd::string_view ScriptEventReceiverNodeDescriptorComponent::GetBusName() const
    {
        return m_busName;
    }

    GraphCanvas::WrappedNodeConfiguration ScriptEventReceiverNodeDescriptorComponent::GetEventConfiguration(const ScriptCanvas::EBusEventId& eventId) const
    {
        AZ_Warning("ScriptCanvas", m_scriptCanvasId.IsValid(), "Trying to query event list before the node is added to the scene.");
        
        GraphCanvas::WrappedNodeConfiguration wrappedConfiguration;

        ScriptCanvas::Nodes::Core::ReceiveScriptEvent* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(m_scriptCanvasId);
        if (eventHandler)
        {
            const ScriptCanvas::Nodes::Core::Internal::ScriptEventBase::EventMap& events = eventHandler->GetEvents();

            AZ::u32 i = 0;
            for (const auto& event : events)
            {
                if (event.first  == eventId)
                {
                    wrappedConfiguration.m_layoutOrder = i;
                    break;
                }

                ++i;
            }
        }

        return wrappedConfiguration;
    }

    AZStd::vector< HandlerEventConfiguration > ScriptEventReceiverNodeDescriptorComponent::GetEventConfigurations() const
    {
        AZStd::vector< HandlerEventConfiguration > configurations;

        AZ_Warning("ScriptCanvas", m_scriptCanvasId.IsValid(), "Trying to query event list before the node is added to the scene.");

        ScriptCanvas::Nodes::Core::ReceiveScriptEvent* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(m_scriptCanvasId);
        if (eventHandler)
        {
            for (auto& event : eventHandler->GetEvents())
            {
                HandlerEventConfiguration configuration;
                configuration.m_eventId = event.first;
                configuration.m_eventName = event.second.m_eventName;
                configurations.emplace_back(configuration);
            }
        }
        return configurations;
    }

    bool ScriptEventReceiverNodeDescriptorComponent::ContainsEvent(const ScriptCanvas::EBusEventId& eventId) const
    {
        return m_eventTypeToId.count(eventId) != 0;
    }

    AZ::EntityId ScriptEventReceiverNodeDescriptorComponent::FindEventNodeId(const ScriptCanvas::EBusEventId& eventId) const
    {
        auto eventTypeEntry = m_eventTypeToId.find(eventId);
        if (eventTypeEntry != m_eventTypeToId.end())
        {
            return eventTypeEntry->second;
        }

        return AZ::EntityId();
    }

    AZ::EntityId ScriptEventReceiverNodeDescriptorComponent::FindGraphCanvasNodeIdForSlot(const ScriptCanvas::SlotId& slotId) const
    {
        ScriptCanvas::Nodes::Core::ReceiveScriptEvent* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(m_scriptCanvasId);
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
                foundResult = foundResult || (AZStd::find(scriptEvent.m_parameterSlotIds.begin(), scriptEvent.m_parameterSlotIds.end(), slotId) != scriptEvent.m_parameterSlotIds.end());

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

    GraphCanvas::Endpoint ScriptEventReceiverNodeDescriptorComponent::MapSlotToGraphCanvasEndpoint(const ScriptCanvas::SlotId& scriptCanvasSlotId) const
    {
        GraphCanvas::Endpoint endpoint;

        AZ::EntityId graphCanvasSlotId;
        SlotMappingRequestBus::EventResult(graphCanvasSlotId, GetEntityId(), &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSlotId);

        if (!graphCanvasSlotId.IsValid())
        {
            for (auto& mapPair : m_eventTypeToId)
            {
                AZ::EntityId graphCanvasSlotId2;
                SlotMappingRequestBus::EventResult(graphCanvasSlotId2, mapPair.second, &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSlotId);

                if (graphCanvasSlotId2.IsValid())
                {
                    endpoint = GraphCanvas::Endpoint(mapPair.second, graphCanvasSlotId2);
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

    void ScriptEventReceiverNodeDescriptorComponent::OnWrappedNode(const AZ::EntityId& wrappedNode)
    {
        AZStd::string eventName;
        ScriptEventReceiverEventNodeDescriptorBus::EventResult(eventName, wrappedNode, &ScriptEventReceiverEventNodeDescriptorRequests::GetEventName);

        ScriptCanvas::EBusEventId eventId;
        EBusHandlerEventNodeDescriptorRequestBus::EventResult(eventId, wrappedNode, &EBusHandlerEventNodeDescriptorRequests::GetEventId);
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
                m_saveData.m_enabledEvents.push_back(AZStd::make_pair(eventId, eventName));
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

    void ScriptEventReceiverNodeDescriptorComponent::OnUnwrappedNode(const AZ::EntityId& unwrappedNode)
    {
        auto iter = m_idToEventType.find(unwrappedNode);

        if (iter != m_idToEventType.end())
        {
            ScriptCanvas::EBusEventId eventId = iter->second;

            m_eventTypeToId.erase(iter->second);
            m_idToEventType.erase(iter);

            for (auto eventIter = m_saveData.m_enabledEvents.begin(); eventIter != m_saveData.m_enabledEvents.end(); ++eventIter)
            {
                if (eventIter->first == eventId)
                {
                    m_saveData.m_enabledEvents.erase(eventIter);
                    m_saveData.SignalDirty();
                    break;
                }
            }
        }
    }

    GraphCanvas::WrappedNodeConfiguration ScriptEventReceiverNodeDescriptorComponent::GetWrappedNodeConfiguration(const AZ::EntityId& wrappedNodeId) const
    {
        ScriptEvents::Method methodDefinition;
        ScriptEventReceiverEventNodeDescriptorBus::EventResult(methodDefinition, wrappedNodeId, &ScriptEventReceiverEventNodeDescriptorRequests::GetMethodDefinition);

        if (methodDefinition.GetName().empty())
        {
            return GraphCanvas::WrappedNodeConfiguration();
        }
        else
        {
            return GetEventConfiguration(AZ::Crc32(methodDefinition.GetNameProperty().GetId().ToString<AZStd::string>().c_str()));
        }
    }

    AZ::Component* ScriptEventReceiverNodeDescriptorComponent::GetPropertyComponent()
    {
        return this;
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        EditorGraphRequestBus::Event(scriptCanvasId, &EditorGraphRequests::QueueVersionUpdate, GetEntityId());
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnVersionConversionBegin()
    {
        DynamicSlotRequestBus::Event(GetEntityId(), &DynamicSlotRequests::StartQueueSlotUpdates);
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnVersionConversionEnd()
    {
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);        

        AZStd::vector< GraphCanvas::NodeId > wrappedNodes;
        GraphCanvas::WrapperNodeRequestBus::EventResult(wrappedNodes, GetEntityId(), &GraphCanvas::WrapperNodeRequests::GetWrappedNodeIds);

        AZStd::unordered_set< AZ::EntityId > deletedNodes;
        deletedNodes.insert(wrappedNodes.begin(), wrappedNodes.end());

        AZStd::vector<AZStd::pair<ScriptCanvas::EBusEventId, AZStd::string>> enabledEvents = m_saveData.m_enabledEvents;
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, deletedNodes);
        
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventsAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();

        UpdateTitles(asset);

        ScriptEventReceiveNodeDescriptorNotificationBus::Event(GetEntityId(), &ScriptEventReceiveNodeDescriptorNotifications::OnScriptEventReloaded, asset);

        const ScriptEvents::ScriptEvent& definition = asset.Get()->m_definition;

        for (auto eventToRecreate : enabledEvents)
        {
            ScriptEvents::Method outMethod;
            if (definition.FindMethod(eventToRecreate.first, outMethod))
            {
                AZ::EntityId graphCanvasNodeId =  Nodes::DisplayScriptEventNode(graphCanvasGraphId, m_scriptEventsAssetId, outMethod);

                if (graphCanvasNodeId.IsValid())
                {
                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Add, graphCanvasNodeId, false);

                    GraphCanvas::WrappedNodeConfiguration configuration = GetWrappedNodeConfiguration(graphCanvasNodeId);
                    GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::WrapNode, graphCanvasNodeId, configuration);
                }
            }
        }
        

        DynamicSlotRequestBus::Event(GetEntityId(), &DynamicSlotRequests::StopQueueSlotUpdates);
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId&, const AZ::EntityId& scriptCanvasNodeId)
    {
        m_scriptCanvasId = scriptCanvasNodeId;

        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetActionString, "Add/Remove Events");
        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_saveData.m_displayConnections);

        if (m_scriptCanvasId.IsValid())
        {
            EditorNodeNotificationBus::Handler::BusConnect(m_scriptCanvasId);

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_scriptCanvasId);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::ReceiveScriptEvent* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(entity);

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
                            key << ScriptCanvasEditor::TranslationHelper::GlobalKeys::EBusHandlerIDKey << "details";
                            GraphCanvas::TranslationRequests::Details details;
                            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);
                            GraphCanvas::SlotRequestBus::Event(testSlotId, &GraphCanvas::SlotRequests::SetDetails, details.m_name, details.m_tooltip);
                            break;
                        }
                    }
                }
            }
        }

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventsAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();

        if (asset.IsReady())
        {
            UpdateTitles(asset);
        }
    }

    void ScriptEventReceiverNodeDescriptorComponent::OnDisplayConnectionsChanged()
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

        if (entity)
        {
            ScriptCanvas::Nodes::Core::ReceiveScriptEvent* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(entity);

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

    void ScriptEventReceiverNodeDescriptorComponent::UpdateTitles(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>& asset)
    {
        if (asset.IsReady())
        {
            const ScriptEvents::ScriptEvent& definition = asset.Get()->m_definition;

            GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, definition.GetTooltip());
            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, definition.GetName());
        }
    }
}
