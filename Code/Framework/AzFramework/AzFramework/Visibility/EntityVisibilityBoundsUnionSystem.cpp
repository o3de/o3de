/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include "EntityVisibilityBoundsUnionSystem.h"

#include <AzFramework/Visibility/BoundsBus.h>
#include <cstring>

namespace AzFramework
{
    void EntityVisibilityBoundsUnionSystem::Connect()
    {
        EntityBoundsUnionRequestBus::Handler::BusConnect();
        AZ::TransformNotificationBus::Router::BusRouterConnect();
        AZ::EntitySystemBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void EntityVisibilityBoundsUnionSystem::Disconnect()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Router::BusRouterDisconnect();
        EntityBoundsUnionRequestBus::Handler::BusDisconnect();
    }

    static void SetUserDataEntityId(VisibilityEntry& visibilityEntry, const AZ::EntityId entityId)
    {
        static_assert(
            sizeof(AZ::EntityId) <= sizeof(visibilityEntry.m_userData), "Ensure EntityId fits into m_userData");

        visibilityEntry.m_typeFlags = VisibilityEntry::TYPE_Entity;

        std::memcpy(&visibilityEntry.m_userData, &entityId, sizeof(AZ::EntityId));
    }

    void EntityVisibilityBoundsUnionSystem::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        // ignore any entity that might activate which does not have a TransformComponent
        if (!AZ::TransformBus::HasHandlers(entityId))
        {
            return;
        }

        if (auto instance_it = m_entityVisibilityBoundsUnionInstanceMapping.find(entityId);
            instance_it == m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            EntityVisibilityBoundsUnionInstance instance;
            instance.m_worldTransform = worldFromLocal;
            instance.m_localEntityBoundsUnion = CalculateEntityLocalBoundsUnion(entityId);
            SetUserDataEntityId(instance.m_visibilityEntry, entityId);

            auto next_it = m_entityVisibilityBoundsUnionInstanceMapping.insert({entityId, instance});
            UpdateVisibilitySystem(next_it.first->second);
        }
    }

    void EntityVisibilityBoundsUnionSystem::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        // ignore any entity that might deactivate which does not have a TransformComponent
        if (!AZ::TransformBus::HasHandlers(entityId))
        {
            return;
        }

        if (auto instance_it = m_entityVisibilityBoundsUnionInstanceMapping.find(entityId);
            instance_it != m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            if (IVisibilitySystem* visibilitySystem = AZ::Interface<IVisibilitySystem>::Get())
            {
                visibilitySystem->GetDefaultVisibilityScene()->RemoveEntry(instance_it->second.m_visibilityEntry);
                m_entityVisibilityBoundsUnionInstanceMapping.erase(instance_it);
            }
        }
    }

    void EntityVisibilityBoundsUnionSystem::UpdateVisibilitySystem(EntityVisibilityBoundsUnionInstance& instance)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        if (const auto& localEntityBoundsUnions = instance.m_localEntityBoundsUnion; localEntityBoundsUnions.IsValid())
        {
            // note: worldEntityBounds will not be a 'tight-fit' Aabb but that of a transformed local aabb
            // there will be some wasted space but it should be sufficient for the visibility system
            const AZ::Aabb worldEntityBoundsUnion =
                localEntityBoundsUnions.GetTransformedAabb(instance.m_worldTransform);
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
        // track entities that need their bounds union to be recalculated
        m_entityIdsBoundsDirty.insert(entityId);
    }

    AZ::Aabb EntityVisibilityBoundsUnionSystem::GetEntityLocalBoundsUnion(const AZ::EntityId entityId) const
    {
        // if the EntityId is not found in the mapping then return a null Aabb, this is to mimic 
        // as closely as possible the behavior of an individual GetLocalBounds call to an Entity that
        // had been deleted (there would be no response, leaving the default value assigned)
        if (auto instance_it = m_entityVisibilityBoundsUnionInstanceMapping.find(entityId);
            instance_it != m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            return instance_it->second.m_localEntityBoundsUnion;
        }

        return AZ::Aabb::CreateNull();
    }

    void EntityVisibilityBoundsUnionSystem::ProcessEntityBoundsUnionRequests()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        // iterate over all entities whose bounds changed and recalculate them
        for (const auto& entityId : m_entityIdsBoundsDirty)
        {
            if (auto instance_it = m_entityVisibilityBoundsUnionInstanceMapping.find(entityId);
                instance_it != m_entityVisibilityBoundsUnionInstanceMapping.end())
            {
                instance_it->second.m_localEntityBoundsUnion = CalculateEntityLocalBoundsUnion(entityId);
            }
        }

        auto allDirtyEntityIds = m_entityIdsTransformDirty;
        allDirtyEntityIds.insert(m_entityIdsBoundsDirty.begin(), m_entityIdsBoundsDirty.end());

        for (const auto& dirtyEntityId : allDirtyEntityIds)
        {
            if (auto instance_it = m_entityVisibilityBoundsUnionInstanceMapping.find(dirtyEntityId);
                instance_it != m_entityVisibilityBoundsUnionInstanceMapping.end())
            {
                UpdateVisibilitySystem(instance_it->second);
            }
        }

        // clear dirty entities once the visibility system has been updated
        m_entityIdsBoundsDirty.clear();
        m_entityIdsTransformDirty.clear();
    }

    void EntityVisibilityBoundsUnionSystem::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        const AZ::EntityId entityId = *AZ::TransformNotificationBus::GetCurrentBusId();
        m_entityIdsTransformDirty.insert(entityId);

        // update the world transform of the visibility bounds union
        if (auto instance_it = m_entityVisibilityBoundsUnionInstanceMapping.find(entityId);
            instance_it != m_entityVisibilityBoundsUnionInstanceMapping.end())
        {
            instance_it->second.m_worldTransform = world;
        }
    }

    void EntityVisibilityBoundsUnionSystem::OnTick(
        [[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ProcessEntityBoundsUnionRequests();
    }
} // namespace AzFramework
