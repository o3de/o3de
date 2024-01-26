/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(CARBONATED)

#include <AzCore/Component/EntityBus.h>
#include <LmbrCentral/Scripting/PrefabSpawnerComponentBus.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace LmbrCentral
{
    /**
    * PrefabSpawnerComponent
    *
    * PrefabSpawnerComponent facilitates spawning of a design-time selected or run-time provided prefab ("*.spawnable") at an entity's location with an optional offset.
    */
    class PrefabSpawnerComponent
        : public AZ::Component
        , private PrefabSpawnerComponentRequestBus::Handler
        , private AZ::EntityBus::MultiHandler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT(PrefabSpawnerComponent, PrefabSpawnerComponentTypeId);

        PrefabSpawnerComponent();
        PrefabSpawnerComponent(const AZ::Data::Asset<AzFramework::Spawnable>& prefabAsset, bool spawnOnActivate);
        ~PrefabSpawnerComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        bool ReadInConfig(const AZ::ComponentConfig* prefabSpawnerConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outPrefabSpawnerConfig) const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // PrefabSpawnerComponentRequestBus::Handler
        void SetSpawnablePrefab(const AZ::Data::Asset<AzFramework::Spawnable>& spawnablePrefabAsset) override;
        void SetSpawnablePrefabByAssetId(AZ::Data::AssetId& assetId) override;
        void SetSpawnOnActivate(bool spawnOnActivate) override;
        bool GetSpawnOnActivate() override;
        AzFramework::EntitySpawnTicket Spawn() override;
        AzFramework::EntitySpawnTicket SpawnRelative(const AZ::Transform& relative) override;
        AzFramework::EntitySpawnTicket SpawnAbsolute(const AZ::Transform& world) override;
        AzFramework::EntitySpawnTicket SpawnPrefab(const AZ::Data::Asset<AzFramework::Spawnable>& prefab) override;
        AzFramework::EntitySpawnTicket SpawnPrefabRelative(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& relative) override;
        AzFramework::EntitySpawnTicket SpawnPrefabAbsolute(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& world) override;
        void DestroySpawnedPrefab(AzFramework::EntitySpawnTicket& ticket) override;
        void DestroyAllSpawnedPrefabs() override;
        AZStd::vector<AzFramework::EntitySpawnTicket> GetCurrentlySpawnedPrefabs() override;
        bool HasAnyCurrentlySpawnedPrefabs() override;
        AZStd::vector<AZ::EntityId> GetCurrentEntitiesFromSpawnedPrefab(const AzFramework::EntitySpawnTicket& ticket) override;
        AZStd::vector<AZ::EntityId> GetAllCurrentlySpawnedEntities() override;
        bool IsReadyToSpawn() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityBus::MultiHandler
        void OnEntityDestruction(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetBus::Handler
        //void OnAssetReady(AZ::Data::Asset<AzFramework::Spawnable> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Serialized members
        AZ::Data::Asset<AzFramework::Spawnable> m_prefabAsset;
        bool m_spawnOnActivate = false;
        bool m_destroyOnDeactivate = false;

    private:

        //////////////////////////////////////////////////////////////////////////
        // Private helpers
        AzFramework::EntitySpawnTicket SpawnPrefabInternalAbsolute(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& world);
        AzFramework::EntitySpawnTicket SpawnPrefabInternalRelative(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& relative);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        void OnPrefabInstantiated(AzFramework::EntitySpawnTicket ticket, AzFramework::SpawnableConstEntityContainerView view);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Runtime-only members
        AZStd::vector<AzFramework::EntitySpawnTicket> m_activeTickets; ///< tickets listed in order they were spawned
        AZStd::unordered_map<AZ::EntityId, AzFramework::EntitySpawnTicket> m_entityToTicketMap; ///< map from entity to ticket that spawned it
        AZStd::unordered_map<AzFramework::EntitySpawnTicket, AZStd::unordered_set<AZ::EntityId>> m_ticketToEntitiesMap; ///< map from ticket to entities it spawned
    };
} // namespace LmbrCentral

#endif
