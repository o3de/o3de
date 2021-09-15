/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Spawnable/SpawnedEntityTicketMapperInterface.h>

namespace AzFramework
{
    class SpawnedEntityTicketMapper
        : public SpawnedEntityTicketMapperInterface
    {
    public:
        AZ_RTTI(SpawnedEntityTicketMapper, "{5C803604-E949-44F4-94A4-F835E5794C77}", SpawnedEntityTicketMapperInterface);

        SpawnedEntityTicketMapper();
        ~SpawnedEntityTicketMapper();

        //! Removes the entityId from the spawned entities map if present.
        //! @param entityId The id of the entity to remove.
        void RemoveSpawnedEntity(AZ::EntityId entityId) override;

        //! Adds the entityId,ticket pair to the spawned entities map.
        //! @param entityId The id of the entity to add.
        //! @param ticket The ticket pointer to add.
        void AddSpawnedEntity(AZ::EntityId entityId, void* ticket) override;
    private:
        SpawnableEntitiesDefinition* spawnableEntitiesInterface = nullptr;
        AZStd::unordered_map<AZ::EntityId, void*> m_spawnedEntities;
    };
} // namespace AzFramework
