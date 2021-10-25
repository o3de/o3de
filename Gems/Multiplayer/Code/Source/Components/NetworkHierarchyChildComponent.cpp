/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyBus.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>

namespace Multiplayer
{
    void NetworkHierarchyChildComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkHierarchyChildComponent, NetworkHierarchyChildComponentBase>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NetworkHierarchyChildComponent>(
                    "Network Hierarchy Child", "Declares a network dependency on the root of this hierarchy.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }
        NetworkHierarchyChildComponentBase::Reflect(context);
    }

    void NetworkHierarchyChildComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkTransformComponent"));
    }

    void NetworkHierarchyChildComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NetworkHierarchyChildComponent"));
    }

    void NetworkHierarchyChildComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyChildComponent"));
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyRootComponent"));
    }

    NetworkHierarchyChildComponent::NetworkHierarchyChildComponent()
        : m_childChangedHandler([this](AZ::ChildChangeType type, AZ::EntityId child) { OnChildChanged(type, child); })
        , m_hierarchyRootNetIdChanged([this](NetEntityId rootNetId) {OnHierarchyRootNetIdChanged(rootNetId); })
    {

    }

    void NetworkHierarchyChildComponent::OnInit()
    {
    }

    void NetworkHierarchyChildComponent::OnActivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_isHierarchyEnabled = true;

        HierarchyRootAddEvent(m_hierarchyRootNetIdChanged);
        NetworkHierarchyRequestBus::Handler::BusConnect(GetEntityId());

        if (AzFramework::TransformComponent* transformComponent = GetEntity()->FindComponent<AzFramework::TransformComponent>())
        {
            transformComponent->BindChildChangedEventHandler(m_childChangedHandler);
        }
    }

    void NetworkHierarchyChildComponent::OnDeactivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_isHierarchyEnabled = false;

        if (m_rootEntity)
        {
            if (NetworkHierarchyRootComponent* root = m_rootEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                root->RebuildHierarchy();
            }
        }

        NotifyChildrenHierarchyDisbanded();

        NetworkHierarchyRequestBus::Handler::BusDisconnect();
    }

    bool NetworkHierarchyChildComponent::IsHierarchyEnabled() const
    {
        return m_isHierarchyEnabled;
    }

    bool NetworkHierarchyChildComponent::IsHierarchicalChild() const
    {
        return GetHierarchyRoot() != InvalidNetEntityId;
    }

    AZ::Entity* NetworkHierarchyChildComponent::GetHierarchicalRoot() const
    {
        return m_rootEntity;
    }

    AZStd::vector<AZ::Entity*> NetworkHierarchyChildComponent::GetHierarchicalEntities() const
    {
        if (m_rootEntity)
        {
            return m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities();
        }

        return {};
    }

    void NetworkHierarchyChildComponent::BindNetworkHierarchyChangedEventHandler(NetworkHierarchyChangedEvent::Handler& handler)
    {
        handler.Connect(m_networkHierarchyChangedEvent);
    }

    void NetworkHierarchyChildComponent::BindNetworkHierarchyLeaveEventHandler(NetworkHierarchyLeaveEvent::Handler& handler)
    {
        handler.Connect(m_networkHierarchyLeaveEvent);
    }

    void NetworkHierarchyChildComponent::SetTopLevelHierarchyRootEntity(AZ::Entity* previousHierarchyRoot, AZ::Entity* newHierarchyRoot)
    {
        if (newHierarchyRoot)
        {
            if (m_rootEntity != newHierarchyRoot)
            {
                m_rootEntity = newHierarchyRoot;

                if (HasController() && GetNetBindComponent()->GetNetEntityRole() == NetEntityRole::Authority)
                {
                    NetworkHierarchyChildComponentController* controller = static_cast<NetworkHierarchyChildComponentController*>(GetController());
                    const NetEntityId netRootId = GetNetworkEntityManager()->GetNetEntityIdById(m_rootEntity->GetId());
                    controller->SetHierarchyRoot(netRootId);
                }

                GetNetBindComponent()->SetOwningConnectionId(m_rootEntity->FindComponent<NetBindComponent>()->GetOwningConnectionId());
                m_networkHierarchyChangedEvent.Signal(m_rootEntity->GetId());
            }
        }
        else if ((previousHierarchyRoot && m_rootEntity == previousHierarchyRoot) || !previousHierarchyRoot)
        {
            m_rootEntity = nullptr;

            if (HasController() && GetNetBindComponent()->GetNetEntityRole() == NetEntityRole::Authority)
            {
                NetworkHierarchyChildComponentController* controller = static_cast<NetworkHierarchyChildComponentController*>(GetController());
                controller->SetHierarchyRoot(InvalidNetEntityId);
            }

            GetNetBindComponent()->SetOwningConnectionId(m_previousOwningConnectionId);
            m_networkHierarchyLeaveEvent.Signal();

            NotifyChildrenHierarchyDisbanded();
        }
    }

    void NetworkHierarchyChildComponent::SetOwningConnectionId(AzNetworking::ConnectionId connectionId)
    {
        NetworkHierarchyChildComponentBase::SetOwningConnectionId(connectionId);
        if (IsHierarchicalChild() == false)
        {
            m_previousOwningConnectionId = connectionId;
        }
    }

    void NetworkHierarchyChildComponent::OnChildChanged([[maybe_unused]] AZ::ChildChangeType type, [[maybe_unused]] AZ::EntityId child)
    {
        if (m_rootEntity)
        {
            if (NetworkHierarchyRootComponent* root = m_rootEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                root->RebuildHierarchy();
            }
        }
    }

    void NetworkHierarchyChildComponent::OnHierarchyRootNetIdChanged(NetEntityId rootNetId)
    {
        ConstNetworkEntityHandle rootHandle = GetNetworkEntityManager()->GetEntity(rootNetId);
        if (rootHandle.Exists())
        {
            AZ::Entity* newRoot = rootHandle.GetEntity();
            if (m_rootEntity != newRoot)
            {
                m_rootEntity = newRoot;

                m_previousOwningConnectionId = GetNetBindComponent()->GetOwningConnectionId();
                GetNetBindComponent()->SetOwningConnectionId(m_rootEntity->FindComponent<NetBindComponent>()->GetOwningConnectionId());

                m_networkHierarchyChangedEvent.Signal(m_rootEntity->GetId());
            }
        }
        else
        {
            GetNetBindComponent()->SetOwningConnectionId(m_previousOwningConnectionId);
            m_isHierarchyEnabled = false;
            m_rootEntity = nullptr;
        }
    }

    void NetworkHierarchyChildComponent::NotifyChildrenHierarchyDisbanded()
    {
        AZ::ComponentApplicationRequests* componentApplication = AZ::Interface<AZ::ComponentApplicationRequests>::Get();

        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, GetEntityId(), &AZ::TransformBus::Events::GetChildren);
        for (const AZ::EntityId& childEntityId : allChildren)
        {
            if (const AZ::Entity* childEntity = componentApplication->FindEntity(childEntityId))
            {
                if (auto* hierarchyChildComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>())
                {
                    hierarchyChildComponent->SetTopLevelHierarchyRootEntity(nullptr, nullptr);
                }
                else if (auto* hierarchyRootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>())
                {
                    hierarchyRootComponent->SetTopLevelHierarchyRootEntity(nullptr, nullptr);
                }
            }
        }
    }
}
