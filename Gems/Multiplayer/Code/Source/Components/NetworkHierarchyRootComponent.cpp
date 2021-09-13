/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>

AZ_CVAR(uint32_t, bg_hierarchyEntityMaxLimit, 16, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Maximum allowed size of network entity hierarchies, including top level entity.");

namespace Multiplayer
{
    void NetworkHierarchyRootComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkHierarchyRootComponent, NetworkHierarchyRootComponentBase>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NetworkHierarchyRootComponent>(
                    "Network Hierarchy Root", "Marks the entity as the root of an entity hierarchy.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }
        NetworkHierarchyRootComponentBase::Reflect(context);
    }

    void NetworkHierarchyRootComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkTransformComponent"));
    }

    void NetworkHierarchyRootComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NetworkHierarchyRootComponent"));
    }

    void NetworkHierarchyRootComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyChildComponent"));
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyRootComponent"));
    }

    void NetworkHierarchyRootComponent::OnInit()
    {
    }

    void NetworkHierarchyRootComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_hierarchicalEntities.push_back(GetEntity());

        NetworkHierarchyRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
    }

    void NetworkHierarchyRootComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        NetworkHierarchyRequestBus::Handler::BusDisconnect();

        for (const AZ::Entity* childEntity : m_hierarchicalEntities)
        {
            auto* hierarchyChildComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>();
            auto* hierarchyRootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>();

            if (hierarchyChildComponent)
            {
                hierarchyChildComponent->SetTopLevelHierarchyRootComponent(nullptr);
            }
            if (hierarchyRootComponent)
            {
                hierarchyRootComponent->SetTopLevelHierarchyRootEntity(nullptr);
            }
        }

        m_hierarchicalEntities.clear();
        m_higherRootEntity = nullptr;
    }

    bool NetworkHierarchyRootComponent::IsHierarchicalRoot() const
    {
        if (GetHierarchyRoot() != InvalidNetEntityId)
        {
            return false;
        }

        return true;
    }

    bool NetworkHierarchyRootComponent::IsHierarchicalChild() const
    {
        return !IsHierarchicalRoot();
    }

    AZStd::vector<AZ::Entity*> NetworkHierarchyRootComponent::GetHierarchicalEntities() const
    {
        return m_hierarchicalEntities;
    }

    AZ::Entity* NetworkHierarchyRootComponent::GetHierarchicalRoot() const
    {
        if (m_higherRootEntity)
        {
            return m_higherRootEntity;
        }

        return GetEntity();
    }

    void NetworkHierarchyRootComponent::OnParentChanged([[maybe_unused]] AZ::EntityId oldParent, AZ::EntityId newParent)
    {
        const AZ::EntityId entityBusId = *AZ::TransformNotificationBus::GetCurrentBusId();
        if (GetEntityId() != entityBusId)
        {
            return; // ignore parent changes of child entities
        }

        if (AZ::Entity* parentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newParent))
        {
            if (parentEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                m_higherRootEntity = parentEntity;
                m_hierarchicalEntities.clear();
                AZ::TransformNotificationBus::MultiHandler::BusDisconnect();

                // Should still listen for its events, such as when this root detaches
                AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
            }
        }
        else
        {
            // detached from parent
            RebuildHierarchy();
        }
    }

    void NetworkHierarchyRootComponent::OnChildAdded([[maybe_unused]] AZ::EntityId child)
    {
        // Parent-child notifications from TransformNotificationBus are not reliable enough to avoid duplicate notifications,
        // so we will rebuild from scratch to avoid duplicate entries in @m_hierarchicalEntities.
        RebuildHierarchy();
    }

    void NetworkHierarchyRootComponent::RebuildHierarchy()
    {
        m_hierarchicalEntities.clear();
        m_hierarchicalEntities.push_back(GetEntity()); // add the root itself

        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());

        uint32_t currentEntityCount = aznumeric_cast<uint32_t>(m_hierarchicalEntities.size());
        RecursiveAttachHierarchicalEntities(GetEntityId(), currentEntityCount);
    }

    bool NetworkHierarchyRootComponent::RecursiveAttachHierarchicalEntities(AZ::EntityId underEntity, uint32_t& currentEntityCount)
    {
        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, underEntity, &AZ::TransformBus::Events::GetChildren);

        for (const AZ::EntityId& newChildId : allChildren)
        {
            if (!RecursiveAttachHierarchicalChild(newChildId, currentEntityCount))
            {
                return false;
            }
        }

        return true;
    }

    bool NetworkHierarchyRootComponent::RecursiveAttachHierarchicalChild(AZ::EntityId entity, uint32_t& currentEntityCount)
    {
        if (currentEntityCount >= bg_hierarchyEntityMaxLimit)
        {
            AZLOG_WARN("Entity %s is trying to build a network hierarchy that is too large. bg_hierarchyEntityMaxLimit is currently set to (%u)",
                GetEntity()->GetName().c_str(), static_cast<uint32_t>(bg_hierarchyEntityMaxLimit));
            return false;
        }

        if (AZ::Entity* childEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entity))
        {
            auto* hierarchyChildComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>();
            auto* hierarchyRootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>();

            if (hierarchyChildComponent || hierarchyRootComponent)
            {
                AZ::TransformNotificationBus::MultiHandler::BusConnect(entity);
                m_hierarchicalEntities.push_back(childEntity);
                ++currentEntityCount;

                if (hierarchyChildComponent)
                {
                    hierarchyChildComponent->SetTopLevelHierarchyRootComponent(this);
                }
                else if (hierarchyRootComponent)
                {
                    hierarchyRootComponent->SetTopLevelHierarchyRootEntity(GetEntity());
                }

                if (!RecursiveAttachHierarchicalEntities(entity, currentEntityCount))
                {
                    return false;
                }
            }
        }

        return true;
    }

    void NetworkHierarchyRootComponent::OnChildRemoved(AZ::EntityId childRemovedId)
    {
        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, childRemovedId, &AZ::TransformBus::Events::GetEntityAndAllDescendants);

        for (AZ::EntityId childId : allChildren)
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(childId);

            const AZ::Entity* childEntity = nullptr;

            AZStd::erase_if(m_hierarchicalEntities, [childId, &childEntity](const AZ::Entity* entity)
                {
                    if (entity->GetId() == childId)
                    {
                        childEntity = entity;
                        return true;
                    }

                    return false;
                });

            if (childEntity)
            {
                if (NetworkHierarchyChildComponent* childComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>())
                {
                    childComponent->SetTopLevelHierarchyRootComponent(nullptr);
                }
            }
        }
    }

    void NetworkHierarchyRootComponent::SetTopLevelHierarchyRootEntity(AZ::Entity* hierarchyRoot)
    {
        m_higherRootEntity = hierarchyRoot;
        if (HasController() && GetNetBindComponent()->GetNetEntityRole() == NetEntityRole::Authority)
        {
            NetworkHierarchyChildComponentController* controller = static_cast<NetworkHierarchyChildComponentController*>(GetController());
            if (hierarchyRoot)
            {
                const NetEntityId netRootId = GetNetworkEntityManager()->GetNetEntityIdById(hierarchyRoot->GetId());
                controller->SetHierarchyRoot(netRootId);
            }
            else
            {
                controller->SetHierarchyRoot(InvalidNetEntityId);
            }
        }
    }
}
