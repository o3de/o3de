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
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptAssetRef.h>

namespace AzFramework::Scripts
{
    //! A helper class for direct calls to SpawnableEntitiesInterface that is
    //! reflected with BehaviorContext for interfacing with Lua API
    class SpawnableScriptMediator final
        : public AZ::TickBus::Handler
    {
    public:
        AZ_TYPE_INFO(AzFramework::Scripts::SpawnableScriptMediator, "{9C3118FE-2E78-49BE-BE7A-B41F95B3FCF8}");

        inline static constexpr const char* RootSpawnableRegistryKey = "/Amazon/AzCore/Bootstrap/RootSpawnable";

        SpawnableScriptMediator();
        ~SpawnableScriptMediator();

        static void Reflect(AZ::ReflectContext* context);
        
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! Creates EntitySpawnTicket using provided prefab asset
        EntitySpawnTicket CreateSpawnTicket(const SpawnableScriptAssetRef& spawnableAsset);

        //! Spawns a prefab and places it under level entity
        bool Spawn(EntitySpawnTicket spawnTicket);

        //! Spawns a prefab and places it under parentId entity
        bool SpawnAndParent(EntitySpawnTicket spawnTicket, const AZ::EntityId& parentId);

        //! Spawns a prefab and places it relative to provided parent,
        //! if no parentId is specified then places it in world coordinates
        bool SpawnAndParentAndTransform(
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
        struct SpawnResult
        {
            AZStd::vector<AZ::EntityId> m_entityList;
            EntitySpawnTicket m_spawnTicket;
        };
        struct DespawnResult
        {
            EntitySpawnTicket m_spawnTicket;
        };
        struct CallbackSentinel
        {
        };
        
        using ResultCommand = AZStd::variant<SpawnResult, DespawnResult>;

        void QueueProcessResult(const ResultCommand& resultCommand); 
        void ProcessResults();

        AZStd::vector<ResultCommand> m_resultCommands;
        AZStd::recursive_mutex m_mutex;
        // used to track when SpawnableScriptMediator is destroyed to avoid executing logic in callbacks
        AZStd::shared_ptr<CallbackSentinel> m_sentinel;

        // Maintain a cache of tickets to at least keep 1 reference in the mediator, some script systems
        // may do garbage collection which could lead to unintended despawn due to reference 
        // counts reaching 0
        AZStd::unordered_set<EntitySpawnTicket> m_cachedSpawnTickets;
    };
} // namespace AzFramework
