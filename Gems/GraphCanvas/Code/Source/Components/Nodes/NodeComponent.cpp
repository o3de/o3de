/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QCoreApplication>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Components/Nodes/NodeComponent.h>

#include <Components/GeometryComponent.h>
#include <Components/PersistentIdComponent.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    //////////////////
    // NodeComponent
    //////////////////

    void NodeComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<NodeSaveData>()
            ->Version(1)
            ->Field("HideUnusedSlots", &NodeSaveData::m_hideUnusedSlots)
            ;

        serializeContext->Class<NodeComponent, GraphCanvasPropertyComponent>()
            ->Version(4)
            ->Field("Configuration", &NodeComponent::m_configuration)
            ->Field("Slots", &NodeComponent::m_slots)
            ->Field("UserData", &NodeComponent::m_userData)
            ->FieldFromBase<NodeComponent>("SaveData", &ComponentSaveDataInterface<NodeSaveData>::m_saveData)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<NodeComponent>("Node", "The node's UI representation")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Node's class attributes")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(AZ::Edit::UIHandlers::Default, &NodeComponent::m_configuration, "Configuration", "This node's properties")
            ;
    }

    AZ::Entity* NodeComponent::CreateCoreNodeEntity(const NodeConfiguration& config)
    {
        // Create this Node's entity.
        AZ::Entity* entity = aznew AZ::Entity();

        entity->CreateComponent<NodeComponent>(config);
        entity->CreateComponent<GeometryComponent>();
        entity->CreateComponent<PersistentIdComponent>();

        return entity;
    }

    NodeComponent::NodeComponent()
    {
    }

    NodeComponent::NodeComponent(const NodeConfiguration& config)
        : m_configuration(config)
    {
    }

    NodeComponent::~NodeComponent()
    {
        for (auto& slotRef : m_slots)
        {
            delete slotRef;
        }
    }

    void NodeComponent::Init()
    {
        InitSaveDataInterface(GetEntityId());

        AZ::EntityBus::Handler::BusConnect(GetEntityId());

        for (auto entityRef : m_slots)
        {
            AZ::Entity* slotEntity = entityRef;
            if (slotEntity)
            {
                if (slotEntity->GetState() == AZ::Entity::State::Constructed)
                {
                    slotEntity->Init();
                }
            }
        }
    }

    void NodeComponent::Activate()
    {
    }

    void NodeComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        GroupableSceneMemberRequestBus::Handler::BusDisconnect();
        SceneMemberRequestBus::Handler::BusDisconnect();
        NodeRequestBus::Handler::BusDisconnect();

        for (AZ::Entity* slotEntity : m_slots)
        {
            if (slotEntity)
            {
                if (slotEntity && slotEntity->GetState() == AZ::Entity::State::Active)
                {
                    slotEntity->Deactivate();
                }
            }
        }

        ConnectionNotificationBus::Handler::BusDisconnect();
    }

    void NodeComponent::OnConnectedTo(const AZ::EntityId& /*connectionId*/, const Endpoint& /*endpoint*/)
    {
        UpdateDisabledStateVisuals();
    }

    void NodeComponent::OnDisconnectedFrom(const AZ::EntityId& /*connectionId*/, const Endpoint& /*endpoint*/)
    {
        RootGraphicsItemRequests* itemInterface = RootGraphicsItemRequestBus::FindFirstHandler(GetEntityId());

        if (itemInterface)
        {
            RootGraphicsItemEnabledState enabledState = itemInterface->GetEnabledState();
            RootGraphicsItemEnabledState updatedState = UpdateEnabledState();

            if (updatedState != enabledState)
            {
                AZStd::unordered_set< NodeId > updatedStateSet;
                updatedStateSet.insert(GetEntityId());

                GraphUtils::SetNodesEnabledState(updatedStateSet, updatedState);
            }
        }
    }

    void NodeComponent::OnEntityExists(const AZ::EntityId& /*entityId*/)
    {
        // Temporary version conversion added in 1.xx to add a PersistentId onto the SceneMembers.
        // Remove after a few revisions with warnings about resaving graphs.
        if (AZ::EntityUtils::FindFirstDerivedComponent<PersistentIdComponent>(GetEntityId()) == nullptr)
        {
            AZ::Entity* selfEntity = GetEntity();
            if (selfEntity)
            {
                selfEntity->CreateComponent<PersistentIdComponent>();
            }
        }
    }

    void NodeComponent::OnEntityActivated(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        // Removing the Node properties from the side panel until we decide what we want to show.
        //GraphCanvasPropertyComponent::Activate();

        NodeRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());
        GroupableSceneMemberRequestBus::Handler::BusConnect(GetEntityId());

        for (AZ::Entity* slotEntity : m_slots)
        {
            if (slotEntity)
            {
                if (slotEntity && slotEntity->GetState() == AZ::Entity::State::Init)
                {
                    slotEntity->Activate();
                    SlotRequestBus::Event(slotEntity->GetId(), &SlotRequests::SetNode, GetEntityId());
                    StyleNotificationBus::Event(slotEntity->GetId(), &StyleNotifications::OnStyleChanged);
                }
            }
        }

        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeActivated);
    }

    void NodeComponent::SetScene(const AZ::EntityId& sceneId)
    {
        if (SceneNotificationBus::Handler::BusIsConnected())
        {
            SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnRemovedFromScene, m_sceneId);
            SceneNotificationBus::Handler::BusDisconnect(m_sceneId);
        }

        m_sceneId = sceneId;
        RegisterIds(GetEntityId(), m_sceneId);

        if (m_sceneId.IsValid())
        {
            SceneNotificationBus::Handler::BusConnect(m_sceneId);
            SceneMemberNotificationBus::Handler::BusConnect(m_sceneId);

            SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, m_sceneId);

            OnStylesChanged();

            AZ::EntityId grid;
            SceneRequestBus::EventResult(grid, m_sceneId, &SceneRequests::GetGrid);

            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetGrid, grid);
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, true);
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, true);

            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnAddedToScene, m_sceneId);
        }
    }

    void NodeComponent::ClearScene(const AZ::EntityId& oldSceneId)
    {
        SceneNotificationBus::Handler::BusDisconnect(oldSceneId);

        AZ_Assert(m_sceneId.IsValid(), "This node (ID: %p) is not in a scene (ID: %p)!", GetEntityId(), m_sceneId);
        AZ_Assert(GetEntityId().IsValid(), "This node (ID: %p) doesn't have an Entity!", GetEntityId());

        if (!m_sceneId.IsValid() || !GetEntityId().IsValid())
        {
            return;
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnRemovedFromScene, m_sceneId);
        m_sceneId.SetInvalid();
    }

    void NodeComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);

        if (m_saveData.m_hideUnusedSlots)
        {
            SceneRequests* requests = SceneRequestBus::FindFirstHandler(GetScene());

            if (requests->IsLoading() || requests->IsPasting())
            {
                m_updateSlotState = true;
            }
            else
            {
                HideUnusedSlotsImpl();
            }
        }
    }

    AZ::EntityId NodeComponent::GetScene() const
    {
        return m_sceneId;
    }
    
    void NodeComponent::OnSceneReady()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneReady);
    }

    void NodeComponent::OnStylesChanged()
    {
        for (auto& slotRef : m_slots)
        {
            StyleNotificationBus::Event(slotRef->GetId(), &StyleNotifications::OnStyleChanged);
        }
    }

    void NodeComponent::OnGraphLoadComplete()
    {
        if (m_updateSlotState)
        {
            m_updateSlotState = false;
            if (m_saveData.m_hideUnusedSlots)
            {
                HideUnusedSlotsImpl();
            }
        }
    }

    void NodeComponent::OnPasteEnd()
    {
        if (m_updateSlotState)
        {
            m_updateSlotState = false;
            if (m_saveData.m_hideUnusedSlots)
            {
                HideUnusedSlotsImpl();
            }
        }
    }

    void NodeComponent::SetTooltip(const AZStd::string& tooltip)
    {
        m_configuration.SetTooltip(tooltip);
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnTooltipChanged, m_configuration.GetTooltip());
    }

    void NodeComponent::SetTranslationKeyedTooltip(const TranslationKeyedString& tooltip)
    {
        m_configuration.SetTooltip(tooltip.GetDisplayString());
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnTooltipChanged, m_configuration.GetTooltip());
    }

    void NodeComponent::AddSlot(const AZ::EntityId& slotId)
    {
        AZ_Assert(slotId.IsValid(), "Slot entity (ID: %s) is not valid!", slotId.ToString().data());

        if (SlotRequestBus::FindFirstHandler(slotId))
        {
            auto slotEntity = AzToolsFramework::GetEntityById(slotId);
            auto entry = AZStd::find_if(m_slots.begin(), m_slots.end(), [slotId](AZ::Entity* slot) { return slot->GetId() == slotId; });
            if (entry == m_slots.end() && slotEntity)
            {
                m_slots.emplace_back(slotEntity);
                SlotRequestBus::Event(slotId, &SlotRequests::SetNode, GetEntityId());
                NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnSlotAddedToNode, slotId);
                SlotNotificationBus::MultiHandler::BusConnect(slotId);
            }
        }
        else
        {
            AZ_Assert(false, "Entity (ID: %s) does not implement SlotRequestBus", slotId.ToString().data());
        }
    }

    void NodeComponent::RemoveSlot(const AZ::EntityId& slotId)
    {
        AZ_Assert(slotId.IsValid(), "Slot (ID: %s) is not valid!", slotId.ToString().data());

        auto entry = AZStd::find_if(m_slots.begin(), m_slots.end(), [slotId](AZ::Entity* slot) { return slot->GetId() == slotId; });
        AZ_Assert(entry != m_slots.end(), "Slot (ID: %s) is unknown", slotId.ToString().data());

        if (entry != m_slots.end())
        {
            m_slots.erase(entry);

            QGraphicsLayoutItem* layoutItem = nullptr;
            VisualRequestBus::EventResult(layoutItem, slotId, &VisualRequests::AsGraphicsLayoutItem);

            SlotNotificationBus::MultiHandler::BusDisconnect(slotId);
            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnSlotRemovedFromNode, slotId);
            SlotRequestBus::Event(slotId, &SlotRequests::ClearConnections);
            SlotRequestBus::Event(slotId, &SlotRequests::SetNode, AZ::EntityId());

            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, slotId);
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);

            delete layoutItem;
        }
    }

    AZStd::vector<AZ::EntityId> NodeComponent::GetSlotIds() const
    {
        AZStd::vector<AZ::EntityId> result;
        result.reserve(m_slots.size());

        for (auto slot : m_slots)
        {
            result.emplace_back(slot->GetId());
        }

        return result;
    }

    AZStd::vector<SlotId> NodeComponent::GetVisibleSlotIds() const
    {
        AZStd::vector< SlotId > result;
        result.reserve(m_slots.size());

        for (auto slot : m_slots)
        {
            SlotId slotId = slot->GetId();

            bool isVisible = false;
            SlotGroup slotGroup = SlotGroups::Invalid;

            SlotRequestBus::EventResult(slotGroup, slotId, &SlotRequests::GetSlotGroup);
            SlotLayoutRequestBus::EventResult(isVisible, GetEntityId(), &SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

            if (!isVisible)
            {
                continue;
            }

            result.emplace_back(slotId);
        }

        return result;
    }

    AZStd::vector<SlotId> NodeComponent::FindVisibleSlotIdsByType(const ConnectionType& connectionType, const SlotType& slotType) const
    {
        AZStd::vector<SlotId> result;
        result.reserve(m_slots.size());

        for (auto slot : m_slots)
        {
            SlotId slotId = slot->GetId();

            bool isVisible = false;
            SlotGroup slotGroup = SlotGroups::Invalid;

            SlotRequestBus::EventResult(slotGroup, slotId, &SlotRequests::GetSlotGroup);
            SlotLayoutRequestBus::EventResult(isVisible, GetEntityId(), &SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

            if (!isVisible)
            {
                continue;
            }

            ConnectionType testConnectionType = ConnectionType::CT_Invalid;
            SlotRequestBus::EventResult(testConnectionType, slotId, &SlotRequests::GetConnectionType);

            if (testConnectionType == ConnectionType::CT_Invalid || testConnectionType != connectionType)
            {
                continue;
            }

            SlotType testSlotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(testSlotType, slotId, &SlotRequests::GetSlotType);

            if (testSlotType == SlotTypes::Invalid || testSlotType != slotType)
            {
                continue;
            }

            result.emplace_back(slotId);
        }

        return result;
    }

    bool NodeComponent::HasConnections() const
    {
        bool hasConnections = false;

        for (auto slot : m_slots)
        {
            SlotId slotId = slot->GetId();

            SlotRequestBus::EventResult(hasConnections, slotId, &SlotRequests::HasConnections);

            if (hasConnections)
            {
                break;
            }
        }

        return hasConnections;
    }

    AZStd::any* NodeComponent::GetUserData() 
    {
        return &m_userData;
    }

    bool NodeComponent::IsWrapped() const
    {
        return m_wrappingNode.IsValid();
    }

    void NodeComponent::SetWrappingNode(const AZ::EntityId& wrappingNode)
    {
        if (!wrappingNode.IsValid())
        {
            AZ::EntityId wrappedNodeCache = m_wrappingNode;

            m_wrappingNode = wrappingNode;

            if (wrappedNodeCache.IsValid())
            {
                NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeUnwrapped, wrappedNodeCache);
            }
        }
        else if (!m_wrappingNode.IsValid())
        {
            m_wrappingNode = wrappingNode;

            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeWrapped, wrappingNode);
        }
        else
        {
            AZ_Warning("Graph Canvas", false, "The same node is trying to be wrapped by two objects at once.");
        }
    }

    AZ::EntityId NodeComponent::GetWrappingNode() const
    {
        return m_wrappingNode;
    }

    void NodeComponent::SignalBatchedConnectionManipulationBegin()
    {
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnBatchedConnectionManipulationBegin);
    }

    void NodeComponent::SignalBatchedConnectionManipulationEnd()
    {
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnBatchedConnectionManipulationEnd);
    }

    void NodeComponent::SignalConnectionMoveBegin(const ConnectionId& connectionId)
    {
        ConnectionNotificationBus::Handler::BusConnect(connectionId);
    }

    RootGraphicsItemEnabledState NodeComponent::UpdateEnabledState()
    {
        RootGraphicsItemRequests* graphicsInterface = RootGraphicsItemRequestBus::FindFirstHandler(GetEntityId());

        if (graphicsInterface == nullptr)
        {
            return RootGraphicsItemEnabledState::ES_Enabled;
        }

        if (graphicsInterface && graphicsInterface->GetEnabledState() == RootGraphicsItemEnabledState::ES_Disabled)
        {
            return graphicsInterface->GetEnabledState();
        }

        bool foundDisabledNode = false;
        bool foundEnabledNode = false;

        AZStd::unordered_set< NodeId > connectedNodes;

        for (AZ::Entity* slotEntity : m_slots)
        {
            Endpoint currentEndpoint(GetEntityId(), slotEntity->GetId());

            SlotRequests* slotInterface = SlotRequestBus::FindFirstHandler(slotEntity->GetId());

            if (slotInterface == nullptr)
            {
                continue;
            }

            ConnectionType connectionType = slotInterface->GetConnectionType();
            SlotType slotType = slotInterface->GetSlotType();

            // We only want to follow execution In's for now.
            if (connectionType == CT_Input
                && slotType == SlotTypes::ExecutionSlot)
            {            
                AZStd::vector< ConnectionId > connections;
                SlotRequestBus::EventResult(connections, slotEntity->GetId(), &SlotRequests::GetConnections);                

                for (ConnectionId connectionId : connections)
                {
                    Endpoint otherEndpoint;
                    ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &ConnectionRequests::FindOtherEndpoint, currentEndpoint);

                    if (otherEndpoint.IsValid())
                    {
                        bool isEnabled = false;
                        RootGraphicsItemRequestBus::EventResult(isEnabled, otherEndpoint.GetNodeId(), &RootGraphicsItemRequests::IsEnabled);

                        if (isEnabled)
                        {
                            foundEnabledNode = true;
                            break;
                        }
                        else
                        {
                            foundDisabledNode = true;
                        }
                    }
                }
            }
        }

        if (foundDisabledNode && !foundEnabledNode)
        {
            graphicsInterface->SetEnabledState(RootGraphicsItemEnabledState::ES_PartialDisabled);
        }
        else
        {
            graphicsInterface->SetEnabledState(RootGraphicsItemEnabledState::ES_Enabled);
        }

        return graphicsInterface->GetEnabledState();
    }

    bool NodeComponent::HasHideableSlots() const
    {
        bool canHideSlots = false;

        for (auto slotEntity : m_slots)
        {
            if (GraphUtils::IsSlotHideable(slotEntity->GetId()))
            {
                canHideSlots = true;
                break;
            }
        }

        return canHideSlots;
    }

    bool NodeComponent::IsHidingUnusedSlots() const
    {
        return m_saveData.m_hideUnusedSlots;
    }

    void NodeComponent::ShowAllSlots()
    {
        if (m_saveData.m_hideUnusedSlots)
        {
            for (auto slotEntity : m_slots)
            {
                VisualRequestBus::Event(slotEntity->GetId(), &VisualRequests::SetVisible, true);
            }

            m_saveData.m_hideUnusedSlots = false;
            m_saveData.SignalDirty();
        }
    }

    void NodeComponent::HideUnusedSlots()
    {
        // Always hide the slots to deal with new slots that might have been added. Or previously filled slots that
        // are no unfilled.
        HideUnusedSlotsImpl();

        m_saveData.m_hideUnusedSlots = true;
        m_saveData.SignalDirty();
    }

    void NodeComponent::SignalNodeAboutToBeDeleted()
    {
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeAboutToBeDeleted);
    }

    void NodeComponent::OnMoveFinalized(bool isValidConnection)
    {
        if (isValidConnection)
        {
            UpdateDisabledStateVisuals();
        }

        ConnectionNotificationBus::Handler::BusDisconnect();
    }

    bool NodeComponent::IsGrouped() const
    {
        return !IsWrapped() && m_owningGroupId.IsValid();
    }

    const AZ::EntityId& NodeComponent::GetGroupId() const
    {
        return m_owningGroupId;
    }

    void NodeComponent::RegisterToGroup(const AZ::EntityId& groupId)
    {
        if (!IsWrapped())
        {
            m_owningGroupId = groupId;
            GroupableSceneMemberNotificationBus::Event(GetEntityId(), &GroupableSceneMemberNotifications::OnGroupChanged);
        }
    }

    void NodeComponent::UnregisterFromGroup(const AZ::EntityId& groupId)
    {
        if (m_owningGroupId == groupId)
        {
            m_owningGroupId.SetInvalid();
            GroupableSceneMemberNotificationBus::Event(GetEntityId(), &GroupableSceneMemberNotifications::OnGroupChanged);
        }
    }
    void NodeComponent::RemoveFromGroup()
    {
        if (m_owningGroupId.IsValid())
        {
            NodeGroupRequestBus::Event(m_owningGroupId, &NodeGroupRequests::RemoveElementFromGroup, GetEntityId());
        }
    }

    void NodeComponent::HideUnusedSlotsImpl()
    {
        HideSlotConfig hideConfig;

        for (auto slotEntity : m_slots)
        {
            Endpoint endpoint(GetEntityId(), slotEntity->GetId());

            if (GraphUtils::CanHideEndpoint(endpoint, hideConfig))
            {
                VisualRequestBus::Event(endpoint.GetSlotId(), &VisualRequests::SetVisible, false);
            }
        }
    }

    void NodeComponent::UpdateDisabledStateVisuals()
    {
        RootGraphicsItemRequests* itemInterface = RootGraphicsItemRequestBus::FindFirstHandler(GetEntityId());

        if (itemInterface)
        {
            RootGraphicsItemEnabledState enabledState = itemInterface->GetEnabledState();
            RootGraphicsItemEnabledState updatedState = UpdateEnabledState();

            if (updatedState != enabledState)
            {
                AZStd::unordered_set< NodeId > updatedStateSet;
                updatedStateSet.insert(GetEntityId());

                GraphUtils::SetNodesEnabledState(updatedStateSet, updatedState);
            }
        }
    }
}
