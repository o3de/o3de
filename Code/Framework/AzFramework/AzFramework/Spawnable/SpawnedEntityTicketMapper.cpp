/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/SpawnedEntityTicketMapper.h>

namespace AzFramework
{
    SpawnedEntityTicketMapper::SpawnedEntityTicketMapper()
    {
        spawnableEntitiesInterface = SpawnableEntitiesInterface::Get();
        AZ_Assert(spawnableEntitiesInterface != nullptr, "SpawnableEntitiesInterface is not found.");

        AZ::Interface<SpawnedEntityTicketMapperInterface>::Register(this);
    }

    SpawnedEntityTicketMapper::~SpawnedEntityTicketMapper()
    {
        AZ::Interface<SpawnedEntityTicketMapperInterface>::Unregister(this);
    }

    void SpawnedEntityTicketMapper::RemoveSpawnedEntity(AZ::EntityId entityId)
    {
        auto spawnedGameEntitiesIterator = m_spawnedEntities.find(entityId);
        if (spawnedGameEntitiesIterator != m_spawnedEntities.end())
        {
            spawnableEntitiesInterface->ClaimEntity(
                spawnedGameEntitiesIterator->first, spawnedGameEntitiesIterator->second);
            m_spawnedEntities.erase(entityId);
        }
    }

    void SpawnedEntityTicketMapper::AddSpawnedEntity(AZ::EntityId entityId, void* ticket)
    {
        m_spawnedEntities.emplace(entityId, ticket);
    }

}

