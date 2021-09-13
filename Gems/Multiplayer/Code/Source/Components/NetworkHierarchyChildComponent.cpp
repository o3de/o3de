/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
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
                    "Network Hierarchy Child", "declares a network dependency on the root of this hierarchy")
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
        : m_hierarchyRootNetIdChanged([this](NetEntityId rootNetId) {OnHierarchyRootNetIdChanged(rootNetId); })
    {

    }

    void NetworkHierarchyChildComponent::OnInit()
    {
    }

    void NetworkHierarchyChildComponent::OnActivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        HierarchyRootAddEvent(m_hierarchyRootNetIdChanged);
        NetworkHierarchyRequestBus::Handler::BusConnect(GetEntityId());
    }

    void NetworkHierarchyChildComponent::OnDeactivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        NetworkHierarchyRequestBus::Handler::BusDisconnect();
    }

    bool NetworkHierarchyChildComponent::IsHierarchicalChild() const
    {
        if (GetHierarchyRoot() != InvalidNetEntityId)
        {
            return true;
        }

        return false;
    }

    AZ::Entity* NetworkHierarchyChildComponent::GetHierarchicalRoot() const
    {
        return m_hierarchyRootComponent ? m_hierarchyRootComponent->GetEntity() : nullptr;
    }

    AZStd::vector<AZ::Entity*> NetworkHierarchyChildComponent::GetHierarchicalEntities() const
    {
        if (m_hierarchyRootComponent)
        {
            return m_hierarchyRootComponent->GetHierarchicalEntities();
        }

        return {};
    }

    void NetworkHierarchyChildComponent::SetTopLevelHierarchyRootComponent(NetworkHierarchyRootComponent* hierarchyRoot)
    {
        m_hierarchyRootComponent = hierarchyRoot;
        if (HasController() && GetNetBindComponent()->GetNetEntityRole() == NetEntityRole::Authority)
        {
            NetworkHierarchyChildComponentController* controller = static_cast<NetworkHierarchyChildComponentController*>(GetController());
            if (hierarchyRoot)
            {
                const NetEntityId netRootId = GetNetworkEntityManager()->GetNetEntityIdById(hierarchyRoot->GetEntityId());
                controller->SetHierarchyRoot(netRootId);

                NetworkHierarchyNotificationBus::Event(GetEntityId(), &NetworkHierarchyNotificationBus::Events::OnNetworkHierarchyUpdated, hierarchyRoot->GetEntityId());
            }
            else
            {
                controller->SetHierarchyRoot(InvalidNetEntityId);
                NetworkHierarchyNotificationBus::Event(GetEntityId(), &NetworkHierarchyNotificationBus::Events::OnLeavingNetworkHierarchy);
            }
        }
    }

    void NetworkHierarchyChildComponent::OnHierarchyRootNetIdChanged(NetEntityId rootNetId)
    {
        const ConstNetworkEntityHandle rootHandle = GetNetworkEntityManager()->GetEntity(rootNetId);
        if (rootHandle.Exists())
        {
            m_hierarchyRootComponent = rootHandle.FindComponent<NetworkHierarchyRootComponent>();
        }
        else
        {
            m_hierarchyRootComponent = nullptr;
        }
    }
}
