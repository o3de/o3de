/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityVisibilityBoundsUnionSystem.h"

#include <AzCore/Debug/Profiler.h>
#include <AzFramework/Visibility/BoundsBus.h>

AZ_DECLARE_BUDGET(AzFramework);

namespace AzFramework
{
    EntityVisibilityBoundsUnionSystem::EntityVisibilityBoundsUnionSystem()
        : m_entityActivatedEventHandler(
              [this](AZ::Entity* entity)
              {
                  OnEntityActivated(entity);
              })
        , m_entityDeactivatedEventHandler(
              [this](AZ::Entity* entity)
              {
                  OnEntityDeactivated(entity);
              })
    {
    }

    void EntityVisibilityBoundsUnionSystem::Connect()
    {
        AZ::Interface<IEntityBoundsUnion>::Register(this);
        IEntityBoundsUnionRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RegisterEntityActivatedEventHandler(m_entityActivatedEventHandler);
        AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RegisterEntityDeactivatedEventHandler(m_entityDeactivatedEventHandler);
    }

    void EntityVisibilityBoundsUnionSystem::Disconnect()
    {
        m_entityActivatedEventHandler.Disconnect();
        m_entityDeactivatedEventHandler.Disconnect();

        AZ::TickBus::Handler::BusDisconnect();
        IEntityBoundsUnionRequestBus::Handler::BusDisconnect();
        AZ::Interface<IEntityBoundsUnion>::Unregister(this);
    }

    void EntityVisibilityBoundsUnionSystem::OnEntityActivated(AZ::Entity* entity)
    {
        // ignore any entity that might activate which does not have a TransformComponent
        if (entity->GetTransform() == nullptr)
        {
            return;
        }

        if (auto instanceIt = m_entityVisibilityBoundsUnionInstanceMapping.find(entity);
            instanceIt == m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            EntityVisibilityBoundsUnionInstance instance;
            instance.m_localEntityBoundsUnion = CalculateEntityLocalBoundsUnion(entity);
            instance.m_visibilityEntry.m_typeFlags = VisibilityEntry::TYPE_Entity;
            instance.m_visibilityEntry.m_userData = static_cast<void*>(entity);

            auto next_it = m_entityVisibilityBoundsUnionInstanceMapping.insert({ entity, instance });
            UpdateVisibilitySystem(entity, next_it.first->second);
        }
    }

    void EntityVisibilityBoundsUnionSystem::OnEntityDeactivated(AZ::Entity* entity)
    {
        // ignore any entity that might deactivate which does not have a TransformComponent
        if (entity->GetTransform() == nullptr)
        {
            return;
        }

        if (auto instanceIt = m_entityVisibilityBoundsUnionInstanceMapping.find(entity);
            instanceIt != m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            if (IVisibilitySystem* visibilitySystem = AZ::Interface<IVisibilitySystem>::Get())
            {
                visibilitySystem->GetDefaultVisibilityScene()->RemoveEntry(instanceIt->second.m_visibilityEntry);
                m_entityVisibilityBoundsUnionInstanceMapping.erase(instanceIt);
            }
        }
    }

    void EntityVisibilityBoundsUnionSystem::UpdateVisibilitySystem(AZ::Entity* entity, EntityVisibilityBoundsUnionInstance& instance)
    {
        if (const auto& localEntityBoundsUnions = instance.m_localEntityBoundsUnion; localEntityBoundsUnions.IsValid())
        {
            // note: worldEntityBounds will not be a 'tight-fit' Aabb but that of a transformed local aabb
            // there will be some wasted space but it should be sufficient for the visibility system
            AZ::TransformInterface* transformInterface = entity->GetTransform();
            const AZ::Aabb worldEntityBoundsUnion = localEntityBoundsUnions.GetTransformedAabb(transformInterface->GetWorldTM());
            IVisibilitySystem* visibilitySystem = AZ::Interface<IVisibilitySystem>::Get();
            if (visibilitySystem && !worldEntityBoundsUnion.IsClose(instance.m_visibilityEntry.m_boundingVolume))
            {
                instance.m_visibilityEntry.m_boundingVolume = worldEntityBoundsUnion;
                visibilitySystem->GetDefaultVisibilityScene()->InsertOrUpdateEntry(instance.m_visibilityEntry);
            }
        }
    }

    void EntityVisibilityBoundsUnionSystem::RefreshEntityLocalBoundsUnion(const AZ::EntityId entityId)
    {
        if (AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId))
        {
            // track entities that need their bounds union to be recalculated
            m_entityBoundsDirty.insert(entity);
        }
    }

    AZ::Aabb EntityVisibilityBoundsUnionSystem::GetEntityLocalBoundsUnion(const AZ::EntityId entityId) const
    {
        if (AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId))
        {
            // if the entity is not found in the mapping then return a null Aabb, this is to mimic
            // as closely as possible the behavior of an individual GetLocalBounds call to an Entity that
            // had been deleted (there would be no response, leaving the default value assigned)
            if (auto instanceIt = m_entityVisibilityBoundsUnionInstanceMapping.find(entity);
                instanceIt != m_entityVisibilityBoundsUnionInstanceMapping.end())
            {
                return instanceIt->second.m_localEntityBoundsUnion;
            }
        }

        return AZ::Aabb::CreateNull();
    }

    AZ::Aabb EntityVisibilityBoundsUnionSystem::GetEntityWorldBoundsUnion(const AZ::EntityId entityId) const
    {
        if (AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId))
        {
            // if the entity is not found in the mapping then return a null Aabb, this is to mimic
            // as closely as possible the behavior of an individual GetLocalBounds call to an Entity that
            // had been deleted (there would be no response, leaving the default value assigned)
            if (auto instanceIt = m_entityVisibilityBoundsUnionInstanceMapping.find(entity);
                instanceIt != m_entityVisibilityBoundsUnionInstanceMapping.end())
            {
                return instanceIt->second.m_localEntityBoundsUnion.GetTranslated(entity->GetTransform()->GetWorldTranslation());
            }
        }

        return AZ::Aabb::CreateNull();
    }

    void EntityVisibilityBoundsUnionSystem::ProcessEntityBoundsUnionRequests()
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        // iterate over all entities whose bounds changed and recalculate them
        for (const auto& entity : m_entityBoundsDirty)
        {
            if (auto instanceIt = m_entityVisibilityBoundsUnionInstanceMapping.find(entity);
                instanceIt != m_entityVisibilityBoundsUnionInstanceMapping.end())
            {
                instanceIt->second.m_localEntityBoundsUnion = CalculateEntityLocalBoundsUnion(entity);
                UpdateVisibilitySystem(entity, instanceIt->second);
            }
        }

        // clear dirty entities once the visibility system has been updated
        m_entityBoundsDirty.clear();
    }

    void EntityVisibilityBoundsUnionSystem::OnTransformUpdated(AZ::Entity* entity)
    {
        // update the world transform of the visibility bounds union
        if (auto instanceIt = m_entityVisibilityBoundsUnionInstanceMapping.find(entity);
            instanceIt != m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            UpdateVisibilitySystem(entity, instanceIt->second);
        }
    }

    void EntityVisibilityBoundsUnionSystem::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ProcessEntityBoundsUnionRequests();
    }
} // namespace AzFramework
