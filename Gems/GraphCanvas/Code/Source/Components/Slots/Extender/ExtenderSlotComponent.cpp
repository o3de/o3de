/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/Slots/Extender/ExtenderSlotComponent.h>

#include <Components/Slots/Extender/ExtenderSlotLayoutComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>

namespace GraphCanvas
{    
    //////////////////////////
    // ExtenderSlotComponent
    //////////////////////////
    
    void ExtenderSlotComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<ExtenderSlotComponent, SlotComponent>()
                ->Version(SaveVersion::Current)
                ->Field("ExtensionId", &ExtenderSlotComponent::m_extenderId)
            ;
        }
    }
    
    AZ::Entity* ExtenderSlotComponent::CreateExtenderSlot(const AZ::EntityId& nodeId, const ExtenderSlotConfiguration& extenderSlotConfiguration)
    {
        AZ::Entity* entity = SlotComponent::CreateCoreSlotEntity();

        ExtenderSlotComponent* extenderSlot = aznew ExtenderSlotComponent(extenderSlotConfiguration);

        if (!entity->AddComponent(extenderSlot))
        {
            AZ_Error("GraphCanvas", false, "Failed to add ExtenderSlotComponent to entity.");
            delete extenderSlot;
            delete entity;
            return nullptr;
        }

        entity->CreateComponent<ExtenderSlotLayoutComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::ExtenderSlot, nodeId, "");

        SlotConnectionFilterComponent* connectionFilter = entity->CreateComponent<SlotConnectionFilterComponent>();

        SlotTypeFilter* slotTypeFilter = aznew SlotTypeFilter(ConnectionFilterType::Include);
        slotTypeFilter->AddSlotType(SlotTypes::DataSlot);

        connectionFilter->AddFilter(slotTypeFilter);

        ConnectionTypeFilter* connectionTypeFilter = aznew ConnectionTypeFilter(ConnectionFilterType::Include);

        switch (extenderSlot->GetConnectionType())
        {
            case ConnectionType::CT_Input:
                connectionTypeFilter->AddConnectionType(CT_Output);
                break;
            case ConnectionType::CT_Output:
                connectionTypeFilter->AddConnectionType(CT_Input);
                break;
            default:
                break;
        };

        connectionFilter->AddFilter(connectionTypeFilter);

        return entity;
    }
    
    ExtenderSlotComponent::ExtenderSlotComponent()
        : SlotComponent(SlotTypes::ExtenderSlot)        
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            m_slotConfiguration.m_slotGroup = SlotGroups::ExtenderGroup;
        }
    }
    
    ExtenderSlotComponent::ExtenderSlotComponent(const ExtenderSlotConfiguration& extenderSlotConfiguration)
        : SlotComponent(SlotTypes::ExtenderSlot, extenderSlotConfiguration)
        , m_extenderId(extenderSlotConfiguration.m_extenderId)
    {
    }
    
    ExtenderSlotComponent::~ExtenderSlotComponent()
    {
    }
    
    void ExtenderSlotComponent::Init()
    {
        SlotComponent::Init();
    }
    
    void ExtenderSlotComponent::Activate()
    {
        SlotComponent::Activate();
        
        ExtenderSlotRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void ExtenderSlotComponent::Deactivate()
    {
        SlotComponent::Deactivate();
        
        ExtenderSlotRequestBus::Handler::BusDisconnect();
    }
    
    void ExtenderSlotComponent::OnSceneMemberAboutToSerialize(GraphSerialization&)
    {
    }    
    
    void ExtenderSlotComponent::AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        AZ_UNUSED(connectionId);
        AZ_UNUSED(endpoint);
    }

    void ExtenderSlotComponent::RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        AZ_UNUSED(connectionId);
        AZ_UNUSED(endpoint);
    }
    
    void ExtenderSlotComponent::SetNode(const AZ::EntityId& nodeId)
    {
        SlotComponent::SetNode(nodeId);
    }

    SlotConfiguration* ExtenderSlotComponent::CloneSlotConfiguration() const
    {
        ExtenderSlotConfiguration* slotConfiguration = aznew ExtenderSlotConfiguration();

        slotConfiguration->m_extenderId = m_extenderId;

        PopulateSlotConfiguration((*slotConfiguration));

        return slotConfiguration;
    }

    int ExtenderSlotComponent::GetLayoutPriority() const
    {
        return std::numeric_limits<int>::min();
    }

    void ExtenderSlotComponent::SetLayoutPriority(int layoutPriority)
    {
        // Extenders should not have their layout priority changed
        AZ_UNUSED(layoutPriority);
    }

    void ExtenderSlotComponent::OnMoveFinalized(bool isValidConnection)
    {
        ConnectionNotificationBus::Handler::BusDisconnect();

        m_proposedSlot = false;
        m_trackedConnectionId.SetInvalid();

        if (m_createdSlot.IsValid())
        {
            if (isValidConnection)
            {
                NodeId nodeId = GetNode();

                GraphId graphId;
                SceneMemberRequestBus::EventResult(graphId, GetNode(), &SceneMemberRequests::GetScene);                

                GraphModelRequestBus::Event(graphId, &GraphModelRequests::FinalizeExtension, nodeId, m_extenderId);

                m_createdSlot.SetInvalid();
            }
            else
            {
                EraseSlot();
            }
        }
    }

    void ExtenderSlotComponent::OnSourceSlotIdChanged(const SlotId& oldSlotId, const SlotId& /*newSlotId*/)
    {
        if (m_proposedSlot)
        {
            if (oldSlotId == m_createdSlot)
            {
                CleanupProposedSlot();
            }
        }
    }

    void ExtenderSlotComponent::OnTargetSlotIdChanged(const SlotId& oldSlotId, const SlotId& /*newSlotId*/)
    {
        if (m_proposedSlot)
        {
            if (oldSlotId == m_createdSlot)
            {
                CleanupProposedSlot();
            }
        }
    }

    void ExtenderSlotComponent::TriggerExtension()
    {
        // Don't need to track anything. Just create the slot then ignore whatever the return is.
        ConstructSlot(GraphModelRequests::ExtensionRequestReason::UserRequest);

        if (m_createdSlot.IsValid())
        {
            const bool isValidSlot = true;
            OnMoveFinalized(isValidSlot);

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetNode(), &SceneMemberRequests::GetScene);

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
        }

        m_createdSlot.SetInvalid();
    }

    Endpoint ExtenderSlotComponent::ExtendForConnectionProposal(const ConnectionId& connectionId, const Endpoint& endpoint)
    {
        // Don't want to extend if we are already extended.
        if (m_createdSlot.IsValid())
        {
            return Endpoint();
        }

        ConstructSlot(GraphModelRequests::ExtensionRequestReason::ConnectionProposal);

        if (!m_createdSlot.IsValid())
        {
            return Endpoint();
        }

        bool isValidConnection = false;
        SlotRequestBus::EventResult(isValidConnection, m_createdSlot, &SlotRequests::CanCreateConnectionTo, endpoint);

        if (!isValidConnection)
        {
            EraseSlot();
            return Endpoint();
        }

        m_proposedSlot = true;

        m_trackedConnectionId = connectionId;
        ConnectionNotificationBus::Handler::BusConnect(connectionId);

        return Endpoint(GetNode(), m_createdSlot);
    }
    
    void ExtenderSlotComponent::OnFinalizeDisplay()
    {
    }

    void ExtenderSlotComponent::ConstructSlot(GraphModelRequests::ExtensionRequestReason reason)
    {
        if (!m_createdSlot.IsValid())
        {
            NodeId nodeId = GetNode();

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            {
                ScopedGraphUndoBlocker undoBlocker(graphId);
                GraphModelRequestBus::EventResult(m_createdSlot, graphId, &GraphModelRequests::RequestExtension, nodeId, m_extenderId, reason);
            }
        }
    }

    void ExtenderSlotComponent::EraseSlot()
    {
        if (m_createdSlot.IsValid())
        {
            NodeId nodeId = GetNode();

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            {
                ScopedGraphUndoBlocker undoBlocker(graphId);
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RemoveSlot, Endpoint(nodeId, m_createdSlot));
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::ExtensionCancelled, nodeId, m_extenderId);
            }

            m_createdSlot.SetInvalid();
        }
    }

    void ExtenderSlotComponent::CleanupProposedSlot()
    {
        ConnectionNotificationBus::Handler::BusDisconnect();

        m_proposedSlot = false;
        m_trackedConnectionId.SetInvalid();

        EraseSlot();
    }
    
    AZ::Entity* ExtenderSlotComponent::ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection)
    {
        ConstructSlot(GraphModelRequests::ExtensionRequestReason::Internal);

        if (m_createdSlot.IsValid())
        {
            Endpoint otherEndpoint;

            if (GetConnectionType() == CT_Input)
            {
                otherEndpoint = sourceEndpoint;
            }
            else if (GetConnectionType() == CT_Output)
            {
                otherEndpoint = targetEndpoint;
            }

            if (createModelConnection)
            {
                SlotRequestBus::EventResult(m_trackedConnectionId, m_createdSlot, &SlotRequests::CreateConnectionWithEndpoint, otherEndpoint);
            }
            else
            {
                SlotRequestBus::EventResult(m_trackedConnectionId, m_createdSlot, &SlotRequests::DisplayConnectionWithEndpoint, otherEndpoint);
            }

            AZ::Entity* connectionEntity = nullptr;

            if (m_trackedConnectionId.IsValid())
            {
                ConnectionNotificationBus::Handler::BusConnect(m_trackedConnectionId);

                AZ::ComponentApplicationBus::BroadcastResult(connectionEntity, &AZ::ComponentApplicationRequests::FindEntity, m_trackedConnectionId);
            }

            return connectionEntity;
        }

        return nullptr;
    }
}
