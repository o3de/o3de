/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/Component/TransformBus.h"

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/SimplePlayerSpawnerComponent.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Multiplayer
{
    void SimplePlayerSpawnerComponent::Activate()
    {
        AZ::Interface<IMultiplayerSpawner>::Register(this);
        AZ::Interface<ISimplePlayerSpawner>::Register(this);
    }

    void SimplePlayerSpawnerComponent::Deactivate()
    {
        AZ::Interface<ISimplePlayerSpawner>::Unregister(this);
        AZ::Interface<IMultiplayerSpawner>::Unregister(this);
    }

    void SimplePlayerSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SimplePlayerSpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("PlayerSpawnable", &SimplePlayerSpawnerComponent::m_playerSpawnable)
                ->Field("SpawnPoints", &SimplePlayerSpawnerComponent::m_spawnPoints)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SimplePlayerSpawnerComponent>(
                               "Simple Network Player Spawner",
                               "A simple player spawner that comes included with the Multiplayer gem. Attach this component to any level's root entity which needs to spawn a network player."
                                        "If no spawn points are provided the network players will be spawned at the world-space origin.")
                           ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                           ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                           ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SimpleNetworkPlayerSpawner.svg")
                           ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SimpleNetworkPlayerSpawner.svg")
                           ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                           ->DataElement(
                               AZ::Edit::UIHandlers::Default,
                               &SimplePlayerSpawnerComponent::m_playerSpawnable,
                               "Player Spawnable Asset",
                               "The network player spawnable asset which will be spawned for each player that joins.")
                           ->DataElement(
                               AZ::Edit::UIHandlers::Default,
                               &SimplePlayerSpawnerComponent::m_spawnPoints,
                               "Spawn Points",
                               "Networked players will spawn at the spawn point locations in order. If there are more players than spawn points, the new players will round-robin back starting with the first spawn point.")
                ;
            }
        }
    }

    void SimplePlayerSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerSpawnerService"));
    }

    void SimplePlayerSpawnerComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerSpawnerService"));
    }

    AZ::Transform SimplePlayerSpawnerComponent::GetNextSpawnPoint() const
    {
        if (m_spawnPoints.empty())
        {
            return AZ::Transform::Identity();
        }

        if (m_spawnIndex >= m_spawnPoints.size())
        {
            AZ_Assert(false, "SimplePlayerSpawnerComponent has an out-of-bounds m_spawnIndex %i. Please ensure spawn index is always valid.", m_spawnIndex);
            return AZ::Transform::Identity();
        }

        const AZ::EntityId spawnPointEntityId = m_spawnPoints[m_spawnIndex];

        if (!spawnPointEntityId.IsValid())
        {
            AZ_Assert(
                false,
                "Empty spawner entry at m_spawnIndex %i. Please ensure spawn index is always valid.",
                m_spawnIndex);
            return AZ::Transform::Identity();
        }

        AZ::Transform spawnPointTransform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(spawnPointTransform, spawnPointEntityId, &AZ::TransformInterface::GetWorldTM);
        return spawnPointTransform;
    }

    const AZStd::vector<AZ::EntityId>& SimplePlayerSpawnerComponent::GetSpawnPoints() const
    {
        return m_spawnPoints;
    }

    uint32_t SimplePlayerSpawnerComponent::GetSpawnPointCount() const
    {
        return aznumeric_cast<uint32_t>(m_spawnPoints.size());
    }

    uint32_t SimplePlayerSpawnerComponent::GetNextSpawnPointIndex() const
    {
        if (m_spawnPoints.empty())
        {
            return 0;
        }

        if (m_spawnIndex >= m_spawnPoints.size())
        {
            AZ_Assert(false, "SimplePlayerSpawnerComponent has an out-of-bounds m_spawnIndex %i. Please ensure spawn index is always valid.", m_spawnIndex);
            return static_cast<uint32_t>(-1);
        }

        return m_spawnIndex;
    }

    void SimplePlayerSpawnerComponent::SetNextSpawnPointIndex(uint32_t index)
    {
        if (index >= m_spawnPoints.size())
        {
            AZLOG_WARN("SetNextSpawnPointIndex called with out-of-bounds spawn index %i; total spawn points: %i", index, aznumeric_cast<uint32_t>(m_spawnPoints.size()));
            return;
        }

        m_spawnIndex = index;
    }

    NetworkEntityHandle SimplePlayerSpawnerComponent::OnPlayerJoin(
        [[maybe_unused]] uint64_t userId, [[maybe_unused]] const MultiplayerAgentDatum& agentDatum)
    {
        const PrefabEntityId prefabEntityId(AZ::Name(m_playerSpawnable.m_spawnableAsset.GetHint().c_str()));

        const AZ::Transform transform = GetNextSpawnPoint();
        m_spawnIndex = ++m_spawnIndex % m_spawnPoints.size();

        INetworkEntityManager::EntityList entityList =
            GetNetworkEntityManager()->CreateEntitiesImmediate(prefabEntityId, NetEntityRole::Authority, transform);

        NetworkEntityHandle controlledEntity;
        if (entityList.empty())
        {
            // Failure: The player prefab has no networked entities in it.
            AZLOG_ERROR(
                "Attempt to spawn prefab '%s' failed, no entities were spawned. Ensure that the prefab contains a single entity "
                "that is network enabled with a Network Binding component.",
                prefabEntityId.m_prefabName.GetCStr());
        }

        controlledEntity = entityList[0];
        return controlledEntity;
    }

    void SimplePlayerSpawnerComponent::OnPlayerLeave(ConstNetworkEntityHandle entityHandle, [[maybe_unused]] const ReplicationSet& replicationSet, [[maybe_unused]] AzNetworking::DisconnectReason reason)
    {
        // Walk hierarchy backwards to remove all children before parents
        AZStd::vector<AZ::EntityId> hierarchy = entityHandle.GetEntity()->GetTransform()->GetEntityAndAllDescendants();
        for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it)
        {
            const AZ::EntityId hierarchyEntityId = *it;
            ConstNetworkEntityHandle hierarchyEntityHandle = GetNetworkEntityManager()->GetEntity(GetNetworkEntityManager()->GetNetEntityIdById(hierarchyEntityId));
            if (hierarchyEntityHandle)
            {
                AZ::Interface<IMultiplayer>::Get()->GetNetworkEntityManager()->MarkForRemoval(hierarchyEntityHandle);
            }
        }
    }
} // namespace Multiplayer
