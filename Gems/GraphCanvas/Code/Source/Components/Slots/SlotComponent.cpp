/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>

#include <Components/Slots/SlotComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Slots/Default/DefaultSlotLayoutComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>


namespace GraphCanvas
{
    //////////////////
    // SlotComponent
    //////////////////

    constexpr int k_defaultPriority = 10;

    void SlotComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<SlotConfiguration>()
            ->Version(2)
            ->Field("ConnectionType", &SlotConfiguration::m_connectionType)
            ->Field("Name", &SlotConfiguration::m_name)
            ->Field("SlotGroup", &SlotConfiguration::m_slotGroup)
            ->Field("ToolTip", &SlotConfiguration::m_tooltip)
            ->Field("IsNameHidden", &SlotConfiguration::m_isNameHidden)
        ;

        serializeContext->Class<SlotComponent, AZ::Component>()
            ->Version(4)
            ->Field("Configuration", &SlotComponent::m_slotConfiguration)
            ->Field("UserData", &SlotComponent::m_userData)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<SlotConfiguration>("Slot Configuration", "The slot's properties")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Slot class attributes")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SlotConfiguration::m_tooltip)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
        ;
    }

    AZ::Entity* SlotComponent::CreateCoreSlotEntity()
    {
        AZ::Entity* entity = aznew AZ::Entity("Slot");
        return entity;
    }

    SlotComponent::SlotComponent()
        : m_layoutPriority(k_defaultPriority)
    {
    }

    SlotComponent::SlotComponent(const SlotType& slotType)
        : m_slotType(slotType)
        , m_layoutPriority(k_defaultPriority)
    {
    }

    SlotComponent::SlotComponent(const SlotType& slotType, const SlotConfiguration& configuration)
        : m_slotType(slotType)
        , m_slotConfiguration(configuration)
        , m_layoutPriority(k_defaultPriority)
    {
    }

    void SlotComponent::Activate()
    {
        SlotRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SlotComponent::Deactivate()
    {
        SceneMemberRequestBus::Handler::BusDisconnect();
        SlotRequestBus::Handler::BusDisconnect();
    }

    void SlotComponent::SetScene(const AZ::EntityId&)
    {
        AZ_Error("Graph Canvas", false, "The scene cannot be set directly on a slot; it follows that of the node to which it belongs (slot: %s)", GetEntityId().ToString().data());
    }

    void SlotComponent::ClearScene(const AZ::EntityId&)
    {
        AZ_Error("Graph Canvas", false, "The scene cannot be cleared directly on a slot; it follows that of the node to which it belongs (slot: %s)", GetEntityId().ToString().data());
    }

    void SlotComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);
    }

    AZ::EntityId SlotComponent::GetScene() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, m_nodeId, &SceneMemberRequests::GetScene);
        return sceneId;
    }

    void SlotComponent::OnSceneSet(const AZ::EntityId& sceneId)
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, sceneId);
    }

    void SlotComponent::OnSceneReady()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneReady);
        FinalizeDisplay();
    }

    const AZ::EntityId& SlotComponent::GetNode() const
    {
        return m_nodeId;
    }

    void SlotComponent::SetNode(const AZ::EntityId& nodeId)
    {
        if (m_nodeId != nodeId)
        {
            m_nodeId = nodeId;

            SceneMemberNotificationBus::Handler::BusDisconnect();
            SceneMemberNotificationBus::Handler::BusConnect(m_nodeId);
            AZ::EntityId sceneId = GetScene();
            if (sceneId.IsValid())
            {
                OnSceneSet(sceneId);
            }

            SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnRegisteredToNode, m_nodeId);
        }
    }

    Endpoint SlotComponent::GetEndpoint() const
    {
        return Endpoint(GetNode(), GetEntityId());
    }

    void SlotComponent::SetName(const AZStd::string& name)
    {
        if (name == m_slotConfiguration.m_name)
        {
            return;
        }

        m_slotConfiguration.m_name = name;

        // Default tooltip.
        if (m_slotConfiguration.m_tooltip.empty())
        {
            m_slotConfiguration.m_tooltip = m_slotConfiguration.m_name;
        }

        SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnNameChanged, m_slotConfiguration.m_name);
    }

    void SlotComponent::SetDetails(const AZStd::string& name, const AZStd::string& tooltip)
    {
        if (name != m_slotConfiguration.m_name)
        {
            m_slotConfiguration.m_name = name;
            SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnNameChanged, m_slotConfiguration.m_name);
        }

        if (tooltip != m_slotConfiguration.m_tooltip)
        {
            m_slotConfiguration.m_tooltip = tooltip;
            SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnTooltipChanged, m_slotConfiguration.m_tooltip);
        }
    }

    void SlotComponent::SetTooltip(const AZStd::string& tooltip)
    {
        if (tooltip == m_slotConfiguration.m_tooltip)
        {
            return;
        }

        m_slotConfiguration.m_tooltip = tooltip;

        // Default tooltip.
        if (m_slotConfiguration.m_tooltip.empty())
        {
            m_slotConfiguration.m_tooltip = m_slotConfiguration.m_name;
        }

        SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnTooltipChanged, m_slotConfiguration.m_tooltip);
    }

    void SlotComponent::DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& /*endpoint*/)
    {
        bool needsStyleUpdate = m_connections.empty();
        m_connections.emplace_back(connectionId);

        if (needsStyleUpdate)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotifications::OnStyleChanged);
        }
    }

    void SlotComponent::RemoveProposedConnection(const AZ::EntityId& connectionId, const Endpoint& /*endpoint*/)
    {
        auto it = AZStd::find(m_connections.begin(), m_connections.end(), connectionId);

        if (it != m_connections.end())
        {
            m_connections.erase(it);

            if (m_connections.empty())
            {
                StyleNotificationBus::Event(GetEntityId(), &StyleNotifications::OnStyleChanged);
            }
        }
    }

    void SlotComponent::AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        bool needsStyleUpdate = m_connections.empty();
        m_connections.emplace_back(connectionId);

        if (needsStyleUpdate)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotifications::OnStyleChanged);
        }

        SlotNotificationBus::Event(GetEntityId(), &SlotNotificationBus::Events::OnConnectedTo, connectionId, endpoint);
    }

    void SlotComponent::RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        auto it = AZStd::find(m_connections.begin(), m_connections.end(), connectionId);

        if (it != m_connections.end())
        {
            m_connections.erase(it);

            if (m_connections.empty())
            {
                StyleNotificationBus::Event(GetEntityId(), &StyleNotifications::OnStyleChanged);
            }
        }

        SlotNotificationBus::Event(GetEntityId(), &SlotNotificationBus::Events::OnDisconnectedFrom, connectionId, endpoint);
    }

    void SlotComponent::SetDisplayOrdering(int ordering)
    {
        m_displayOrdering = ordering;
    }

    int SlotComponent::GetDisplayOrdering() const
    {
        return m_displayOrdering;
    }

    bool SlotComponent::IsConnectedTo(const Endpoint& endpoint) const
    {
        bool isConnectedTo = false;
        for (const AZ::EntityId& connection : m_connections)
        {
            ConnectionRequestBus::EventResult(isConnectedTo, connection, &ConnectionRequests::ContainsEndpoint, endpoint);

            if (isConnectedTo)
            {
                break;
            }
        }

        return isConnectedTo;
    }

    void SlotComponent::FindConnectionsForEndpoints(const AZStd::unordered_set<Endpoint>& searchEndpoints, AZStd::unordered_set<ConnectionId>& outConnections)
    {
        for (const AZ::EntityId& connection : m_connections)
        {
            GraphCanvas::Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connection, &ConnectionRequests::GetSourceEndpoint);

            if (searchEndpoints.count(sourceEndpoint) > 0)
            {
                outConnections.insert(connection);
                continue;
            }

            GraphCanvas::Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connection, &ConnectionRequests::GetTargetEndpoint);

            if (searchEndpoints.count(targetEndpoint) > 0)
            {
                outConnections.insert(connection);
                continue;
            }
        }
    }

    bool SlotComponent::CanDisplayConnectionTo(const Endpoint& endpoint) const
    {
        bool isConnectable = false;

        ConnectionMoveType moveType = ConnectionMoveType::Unknown;

        if (GetConnectionType() == CT_Input)
        {
            moveType = ConnectionMoveType::Target;
        }
        else if (GetConnectionType() == CT_Output)
        {
            moveType = ConnectionMoveType::Source;
        }

        ConnectionFilterRequestBus::EventResult(isConnectable, GetEntityId(), &ConnectionFilterRequests::CanConnectWith, endpoint, moveType);

        return isConnectable;
    }

    bool SlotComponent::CanCreateConnectionTo(const Endpoint& endpoint) const
    {
        bool isConnectable = CanDisplayConnectionTo(endpoint);

        if (isConnectable)
        {
            GraphId graphId = GetScene();

            if (GetConnectionType() == CT_Input)
            {
                isConnectable = GraphUtils::IsValidModelConnection(graphId, endpoint, GetEndpoint());
            }
            else if (GetConnectionType() == CT_Output)
            {
                isConnectable = GraphUtils::IsValidModelConnection(graphId, GetEndpoint(), endpoint);
            }
        }

        return isConnectable;
    }

    AZ::EntityId SlotComponent::CreateConnectionWithEndpoint(const Endpoint& otherEndpoint)
    {
        const bool createConnection = true;
        return CreateConnectionHelper(otherEndpoint, createConnection);
    }

    AZ::EntityId SlotComponent::DisplayConnection()
    {
        Endpoint invalidEndpoint;
        return DisplayConnectionWithEndpoint(invalidEndpoint);
    }

    AZ::EntityId SlotComponent::DisplayConnectionWithEndpoint(const Endpoint& otherEndpoint)
    {
        const bool createConnection = false;
        return CreateConnectionHelper(otherEndpoint, createConnection);
    }

    AZStd::any* SlotComponent::GetUserData()
    {
        return &m_userData;
    }

    bool SlotComponent::HasConnections() const
    {
        return m_connections.size() > 0;
    }

    AZ::EntityId SlotComponent::GetLastConnection() const
    {
        if (m_connections.size() > 0)
        {
            return m_connections.back();
        }

        return AZ::EntityId();
    }

    bool SlotComponent::IsNameHidden() const
    {
        return m_slotConfiguration.m_isNameHidden;
    }

    AZStd::vector<AZ::EntityId> SlotComponent::GetConnections() const
    {
        return m_connections;
    }

    void SlotComponent::SetConnectionDisplayState(RootGraphicsItemDisplayState displayState)
    {
        m_connectionDisplayStateStateSetter.ResetStateSetter();

        for (const AZ::EntityId& connectionId : m_connections)
        {
            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
            RootGraphicsItemRequestBus::EventResult(stateController, connectionId, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_connectionDisplayStateStateSetter.AddStateController(stateController);
        }

        m_connectionDisplayStateStateSetter.SetState(displayState);
    }

    void SlotComponent::ReleaseConnectionDisplayState()
    {
        m_connectionDisplayStateStateSetter.ResetStateSetter();
    }

    void SlotComponent::ClearConnections()
    {
        AZStd::unordered_set< AZ::EntityId > deleteIds;

        for (AZ::EntityId connectionId : m_connections)
        {
            deleteIds.insert(connectionId);
        }

        SceneRequestBus::Event(GetScene(), &SceneRequests::Delete, deleteIds);
    }

    const SlotConfiguration& SlotComponent::GetSlotConfiguration() const
    {
        return m_slotConfiguration;
    }

    SlotConfiguration* SlotComponent::CloneSlotConfiguration() const
    {
        SlotConfiguration* slotConfiguration = aznew SlotConfiguration();

        PopulateSlotConfiguration((*slotConfiguration));

        return slotConfiguration;
    }

    void SlotComponent::RemapSlotForModel(const Endpoint& endpoint)
    {
        AZ_Assert(m_modelRedirections.empty(), "Multiple slot redirections is not currently supported with the connection deletion flow in GraphCanvas.");

        auto endpointIter = AZStd::find_if(m_modelRedirections.begin(), m_modelRedirections.end(), [&endpoint](const Endpoint& otherEndpoint) { return endpoint == otherEndpoint; });

        // Want to avoid duplicates in here
        if (endpointIter == m_modelRedirections.end())
        {
            m_modelRedirections.push_back(endpoint);
        }
    }

    bool SlotComponent::HasModelRemapping() const
    {
        return !m_modelRedirections.empty();
    }

    AZStd::vector< Endpoint > SlotComponent::GetRemappedModelEndpoints() const
    {
        return m_modelRedirections;
    }

    int SlotComponent::GetLayoutPriority() const
    {
        return m_layoutPriority;
    }

    void SlotComponent::SetLayoutPriority(int layoutPriority)
    {
        if (m_layoutPriority != layoutPriority)
        {
            m_layoutPriority = layoutPriority;
            
            SlotUINotificationBus::Event(GetEntityId(), &SlotUINotifications::OnSlotLayoutPriorityChanged, layoutPriority);
        }
    }

    void SlotComponent::PopulateSlotConfiguration(SlotConfiguration& slotConfiguration) const
    {
        slotConfiguration.m_connectionType = GetConnectionType();

        slotConfiguration.m_name = m_slotConfiguration.m_name;
        slotConfiguration.m_tooltip = m_slotConfiguration.m_tooltip;

        slotConfiguration.m_slotGroup = GetSlotGroup();
    }

    AZ::EntityId SlotComponent::CreateConnectionHelper(const Endpoint& otherEndpoint, bool createConnection)
    {
        if (createConnection)
        {
            if (otherEndpoint.IsValid() && !CanCreateConnectionTo(otherEndpoint))
            {
                return AZ::EntityId();
            }
        }

        Endpoint sourceEndpoint;
        Endpoint targetEndpoint;

        Endpoint endpoint(GetNode(), GetEntityId());

        if (GetConnectionType() == CT_Input)
        {
            sourceEndpoint = otherEndpoint;
            targetEndpoint = endpoint;
        }
        else
        {
            sourceEndpoint = endpoint;
            targetEndpoint = otherEndpoint;
        }

        AZ::Entity* connectionEntity = ConstructConnectionEntity(sourceEndpoint, targetEndpoint, createConnection);

        if (connectionEntity)
        {
            // Tweak to allow Extenders to just return the previously constructed element to help simplify down the
            // addition process.
            if (connectionEntity->GetState() == AZ::Entity::State::Active)
            {
                return connectionEntity->GetId();
            }

            GraphId graphId = GetScene();
            EditorId editorId;

            SceneRequestBus::EventResult(editorId, graphId, &SceneRequests::GetEditorId);

            AssetEditorRequestBus::Event(editorId, &AssetEditorRequests::CustomizeConnectionEntity, connectionEntity);

            connectionEntity->Init();
            connectionEntity->Activate();

            SceneRequestBus::Event(graphId, &SceneRequests::AddConnection, connectionEntity->GetId());

            return connectionEntity->GetId();
        }

        return AZ::EntityId();
    }

    AZ::Entity* SlotComponent::ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection)
    {
        return ConnectionComponent::CreateGeneralConnection(sourceEndpoint, targetEndpoint, createModelConnection);
    }

    void SlotComponent::FinalizeDisplay()
    {
        SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnNameChanged, m_slotConfiguration.m_name);
        SlotNotificationBus::Event(GetEntityId(), &SlotNotifications::OnTooltipChanged, m_slotConfiguration.m_tooltip);

        OnFinalizeDisplay();
    }

    void SlotComponent::OnFinalizeDisplay()
    {
    }
}
