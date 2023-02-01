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
    }

    void SimplePlayerSpawnerComponent::Deactivate()
    {
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
                           ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/NetBinding.svg")
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

    NetworkEntityHandle SimplePlayerSpawnerComponent::OnPlayerJoin(
        [[maybe_unused]] uint64_t userId, [[maybe_unused]] const MultiplayerAgentDatum& agentDatum)
    {
        const PrefabEntityId prefabEntityId(AZ::Name(m_playerSpawnable.m_spawnableAsset.GetHint().c_str()));
        AZ::Transform transform = AZ::Transform::Identity();

        if (m_spawnPoints.empty())
        {
            AZLOG_WARN("SimplePlayerSpawnerComponent is missing spawn points. Spawning new player at the origin.")
        }
        else
        {
            m_spawnIndex %= m_spawnPoints.size();
            const AZ::EntityId spawnPointEntityId = m_spawnPoints[m_spawnIndex];
            
            AZ::Entity* spawnPointEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                spawnPointEntity, &AZ::ComponentApplicationBus::Events::FindEntity, spawnPointEntityId);
            if (spawnPointEntity != nullptr)
            {
                transform = spawnPointEntity->GetTransform()->GetWorldTM();
            }
            else
            {
                AZLOG_WARN("The spawn point entity id for index %i is invalid. Spawning new player at the origin.", m_spawnIndex)
            }
        }

        ++m_spawnIndex;

        INetworkEntityManager::EntityList entityList =
            GetNetworkEntityManager()->CreateEntitiesImmediate(prefabEntityId, NetEntityRole::Authority, transform);

        NetworkEntityHandle controlledEntity;
        if (!entityList.empty())
        {
            controlledEntity = entityList[0];
        }
        else
        {
            AZLOG_WARN("Attempt to spawn prefab %s failed. Check that prefab is network enabled.", prefabEntityId.m_prefabName.GetCStr());
        }

        return controlledEntity;
    }

    void SimplePlayerSpawnerComponent::OnPlayerLeave(ConstNetworkEntityHandle entityHandle, [[maybe_unused]] const ReplicationSet& replicationSet, [[maybe_unused]] AzNetworking::DisconnectReason reason)
    {
        AZ::Interface<IMultiplayer>::Get()->GetNetworkEntityManager()->MarkForRemoval(entityHandle);
    }
} // namespace SimplePlayerSpawnerComponent
