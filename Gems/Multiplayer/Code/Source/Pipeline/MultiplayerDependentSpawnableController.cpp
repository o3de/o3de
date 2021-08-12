/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Pipeline/MultiplayerDependentSpawnableController.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/INetworkSpawnableLibrary.h>

namespace Multiplayer
{
    AZ::Name MultiplayerDependentSpawnableController::GetControllerName()
    {
        return AZ::Name("MultiplayerDependentSpawnableController");
    }

    AZ::Name MultiplayerDependentSpawnableController::GetName() const
    {
        return GetControllerName();
    }

    void MultiplayerDependentSpawnableController::ProcessSpawnable(
        const AzFramework::Spawnable& dependentSpawnable,
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& entityIdMap,
        AZ::SerializeContext* serializeContext)
    {
        const auto agentType = AZ::Interface<IMultiplayer>::Get()->GetAgentType();
        const bool spawnImmediately =
            (agentType == MultiplayerAgentType::ClientServer || agentType == MultiplayerAgentType::DedicatedServer);

        if (spawnImmediately)
        {
            const AzFramework::Spawnable::EntityList& entitiesToSpawn = dependentSpawnable.GetEntities();

            for (auto& entity : entitiesToSpawn)
            {
                entityIdMap.emplace(entity->GetId(), AZ::Entity::MakeId());
            }

            uint32_t netEntityIndex = 0;

            for (auto& entityTemplate : entitiesToSpawn)
            {
                constexpr bool allowDuplicateIds = false;

                AZ::Entity* entity = AZ::IdUtils::Remapper<AZ::EntityId, allowDuplicateIds>::CloneObjectAndGenerateNewIdsAndFixRefs(
                    entityTemplate.get(), entityIdMap, serializeContext);

                AZ::Name spawnableName =
                    AZ::Interface<INetworkSpawnableLibrary>::Get()->GetSpawnableNameFromAssetId(dependentSpawnable.GetId());
                PrefabEntityId prefabEntityId;
                prefabEntityId.m_prefabName = spawnableName;
                prefabEntityId.m_entityOffset = netEntityIndex;
                AZ::Interface<INetworkEntityManager>::Get()->SetupNetEntity(entity, prefabEntityId, NetEntityRole::Authority);

                AzFramework::GameEntityContextRequestBus::Broadcast(
                    &AzFramework::GameEntityContextRequestBus::Events::AddGameEntity, entity);

                netEntityIndex++;
            }
        }
    }
}
