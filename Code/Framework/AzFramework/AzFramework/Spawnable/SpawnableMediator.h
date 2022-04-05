/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Spawnable/SpawnableAssetRef.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AzFramework
{
    //! A helper class for direct calls to SpawnableEntitiesInterface that is
    //! reflected with BehaviorContext for interfacing with Lua API
    class SpawnableMediator
        : public AZ::TickBus::Handler
    {
    public:
        AZ_TYPE_INFO(AzFramework::SpawnableMediator, "{9C3118FE-2E78-49BE-BE7A-B41F95B3FCF8}");

        inline static constexpr const char* RootSpawnableRegistryKey = "/Amazon/AzCore/Bootstrap/RootSpawnable";

        SpawnableMediator();
        ~SpawnableMediator();

        static void Reflect(AZ::ReflectContext* context);
        
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! Creates EntitySpawnTicket using provided prefab asset
        EntitySpawnTicket CreateSpawnTicket(const SpawnableAssetRef& spawnableAsset);

        //! Spawns a prefab and places it under level entity
        bool Spawn(EntitySpawnTicket spawnTicket);

        //! Spawns a prefab and places it under parentId entity
        bool SpawnAndParent(EntitySpawnTicket spawnTicket, const AZ::EntityId& parentId);

        //! Spawns a prefab and places it relative to provided parent,
        //! if no parentId is specified then places it in world coordinates
        bool SpawnParentAndTransform(
            EntitySpawnTicket spawnTicket,
            const AZ::EntityId& parentId,
            const AZ::Vector3& translation,
            const AZ::Vector3& rotation,
            float scale);

        //! Despawns a prefab
        bool Despawn(EntitySpawnTicket spawnTicket);

        //! Clears delayed spawn or despawn callbacks
        void Clear();

    private:
        struct SpawnableResult
        {
            AZStd::vector<AZ::EntityId> m_entityList;
            EntitySpawnTicket m_spawnTicket;
        };

        void ProcessSpawnedResults();
        void ProcessDespawnedResults();

        AZStd::vector<SpawnableResult> m_spawnedResults;
        AZStd::vector<EntitySpawnTicket> m_despawnedResults;
        AZStd::recursive_mutex m_mutex;
        SpawnableEntitiesDefinition* m_interface;
    };
} // namespace AzFramework
