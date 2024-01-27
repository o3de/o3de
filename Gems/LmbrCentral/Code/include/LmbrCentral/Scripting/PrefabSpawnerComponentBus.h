/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(CARBONATED)

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Helpers/AssetHelpers.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace LmbrCentral
{
    inline constexpr AZ::TypeId PrefabSpawnerComponentTypeId{ "{27DA1208-BB26-5432-C75A-7234A235CA34}" };

    /*!
     * PrefabSpawnerComponentRequests
     * Messages serviced by the PrefabSpawnerComponent.
     */
    class PrefabSpawnerComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~PrefabSpawnerComponentRequests() {}

        //! Set the spawnable prefab
        virtual void SetSpawnablePrefab(const AZ::Data::Asset<AzFramework::Spawnable>& spawnablePrefabAsset) = 0;
        virtual void SetSpawnablePrefabByAssetId(AZ::Data::AssetId& assetId) = 0;

        //! Sets the SpawnOnActivate parameter
        virtual void SetSpawnOnActivate(bool spawnOnActivate) = 0;

        //! Gets the value of the SpawnOnActivate parameter
        virtual bool GetSpawnOnActivate() = 0;

        //! Spawn the selected prefab at the entity's location
        virtual AzFramework::EntitySpawnTicket Spawn() = 0;

        //! Spawn the selected prefab at the entity's location with the provided relative offset
        virtual AzFramework::EntitySpawnTicket SpawnRelative(const AZ::Transform& relative) = 0;

        //! Spawn the selected prefab at the specified world transform
        virtual AzFramework::EntitySpawnTicket SpawnAbsolute(const AZ::Transform& world) = 0;

        //! Spawn the provided prefab at the entity's location
        virtual AzFramework::EntitySpawnTicket SpawnPrefab(const AZ::Data::Asset<AzFramework::Spawnable>& prefab) = 0;

        //! Spawn the provided prefab at the entity's location with the provided relative offset
        virtual AzFramework::EntitySpawnTicket SpawnPrefabRelative(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& relative) = 0;

        //! Spawn the provided prefab at the specified world transform
        virtual AzFramework::EntitySpawnTicket SpawnPrefabAbsolute(const AZ::Data::Asset<AzFramework::Spawnable>& prefab, const AZ::Transform& world) = 0;

        //! Destroy all entities from a spawned prefab.
        //! If the prefab hasn't finished spawning, it is cancelled.
        //! This component can only destroy prefab that it spawned.
        virtual void DestroySpawnedPrefab(AzFramework::EntitySpawnTicket& ticket) = 0;

        //! Destroy all entities that have been spawned by this component
        //! Any prefabs that haven't finished spawning are cancelled.
        virtual void DestroyAllSpawnedPrefabs() = 0;

        //! Returns tickets for spawned prefabs that haven't been destroyed yet.
        //! A prefab is considered destroyed once all its entities are destroyed.
        //! Includes tickets for prefabs that haven't finished spawning yet.
        //! Only prefabs spawned by this component are returned.
        virtual AZStd::vector<AzFramework::EntitySpawnTicket> GetCurrentlySpawnedPrefabs() = 0;

        //! Returns whether this component has spawned any prefabs that haven't been destroyed.
        //! A prefab is considered destroyed once all its entities are destroyed.
        //! Returns true if any prefabs haven't finished spawning yet.
        virtual bool HasAnyCurrentlySpawnedPrefabs() = 0;

        //! Returns the IDs of current entities from a spawned prefab.
        //! Note that spawning is not instant, if the prefab hasn't finished spawning then no entities are returned.
        //! If an entity has been destroyed since it was spawned, its ID is not returned.
        //! Only prefabs spawned by this component can be queried.
        virtual AZStd::vector<AZ::EntityId> GetCurrentEntitiesFromSpawnedPrefab(const AzFramework::EntitySpawnTicket& ticket) = 0;

        //! Returns the IDs of all existing entities spawned by this component.
        //! Note that spawning is not instant, if a prefab hasn't finished spawning then none of its entities are returned.
        //! If an entity has been destroyed since it was spawned, its ID is not returned.
        virtual AZStd::vector<AZ::EntityId> GetAllCurrentlySpawnedEntities() = 0;

        //! Returns whether or not the spawner is in a state that's ready to spawn.
        virtual bool IsReadyToSpawn() = 0;
    };

    using PrefabSpawnerComponentRequestBus = AZ::EBus<PrefabSpawnerComponentRequests>;

    /*!
     * PrefabSpawnerComponentNotificationBus
     * Events dispatched by the PrefabSpawnerComponent
     */
    class PrefabSpawnerComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~PrefabSpawnerComponentNotifications() {}

        //! Notify that prefab has been spawned, but entities have not yet been activated.
        //! OnEntitySpawned events are about to be dispatched.
        virtual void OnSpawnBegin(const AzFramework::EntitySpawnTicket& /*ticket*/) {}

        //! Notify that a spawn has been completed. All OnEntitySpawned events have been dispatched.
        virtual void OnSpawnEnd(const AzFramework::EntitySpawnTicket& /*ticket*/) {}

        //! Notify that an entity has spawned, will be called once for each entity spawned in a prefab.
        virtual void OnEntitySpawned(const AzFramework::EntitySpawnTicket& /*ticket*/, const AZ::EntityId& /*spawnedEntity*/) {}

        //! Single event notification for an entire prefab spawn, providing a list of all resulting entity Ids.
        virtual void OnEntitiesSpawned(const AzFramework::EntitySpawnTicket& /*ticket*/, const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

        //! Notify of a spawned prefab's destruction.
        //! This occurs when all entities from a spawn are destroyed, or the prefab fails to spawn.
        virtual void OnSpawnedPrefabDestroyed(const AzFramework::EntitySpawnTicket& /*ticket*/) {}
    };

    using PrefabSpawnerComponentNotificationBus = AZ::EBus<PrefabSpawnerComponentNotifications>;

    class PrefabSpawnerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(PrefabSpawnerConfig, "{A2BC3452-A345-00AB-129E-B457ACD38AAA}", AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(PrefabSpawnerConfig, AZ::SystemAllocator);

        PrefabSpawnerConfig() = default;

        ///The prefab asset to be spawned
        AZ::Data::Asset<AzFramework::Spawnable> m_prefabAsset { AZ::Data::AssetLoadBehavior::PreLoad };
        ///Whether or not to spawn the prefab when the component activates
        bool m_spawnOnActivate = false;
        ///Whether or not to destroy the prefab when the component deactivates
        bool m_destroyOnDeactivate = false;
    };

} // namespace LmbrCentral
#endif
