/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceInstantiationTicket.h>


namespace LmbrCentral
{
    inline constexpr AZ::TypeId SpawnerComponentTypeId{ "{8022A627-DD76-5432-C75A-7234AC2798C1}" };
    inline constexpr AZ::TypeId DeprecatedSpawnerComponentTypeId{ "{8022A627-FA7D-4516-A155-657A0927A3CA}" };

    /*!
     * SpawnerComponentRequests
     * Messages serviced by the SpawnerComponent.
     */
    class SpawnerComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~SpawnerComponentRequests() {}

        //! Set the dynamic slice 
        virtual void SetDynamicSlice(const AZ::Data::Asset<AZ::DynamicSliceAsset>& dynamicSliceAsset) = 0;
        virtual void SetDynamicSliceByAssetId(AZ::Data::AssetId& assetId) = 0;

        //! Sets the SpawnOnActivate parameter
        virtual void SetSpawnOnActivate(bool spawnOnActivate) = 0;

        //! Gets the value of the SpawnOnActivate parameter
        virtual bool GetSpawnOnActivate() = 0;

        //! Spawn the selected slice at the entity's location
        virtual AzFramework::SliceInstantiationTicket Spawn() = 0;

        //! Spawn the selected slice at the entity's location with the provided relative offset
        virtual AzFramework::SliceInstantiationTicket SpawnRelative(const AZ::Transform& relative) = 0;

        //! Spawn the selected slice at the specified world transform
        virtual AzFramework::SliceInstantiationTicket SpawnAbsolute(const AZ::Transform& world) = 0;

        //! Spawn the provided slice at the entity's location
        virtual AzFramework::SliceInstantiationTicket SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice) = 0;

        //! Spawn the provided slice at the entity's location with the provided relative offset
        virtual AzFramework::SliceInstantiationTicket SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative) = 0;

        //! Spawn the provided slice at the specified world transform
        virtual AzFramework::SliceInstantiationTicket SpawnSliceAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world) = 0;

        //! Destroy all entities from a spawned slice.
        //! If the slice hasn't finished spawning, it is cancelled.
        //! This component can only destroy slices that it spawned.
        virtual void DestroySpawnedSlice(const AzFramework::SliceInstantiationTicket& ticket) = 0;

        //! Destroy all entities that have been spawned by this component
        //! Any slices that haven't finished spawning are cancelled.
        virtual void DestroyAllSpawnedSlices() = 0;

        //! Returns tickets for spawned slices that haven't been destroyed yet.
        //! A slice is considered destroyed once all its entities are destroyed.
        //! Includes tickets for slices that haven't finished spawning yet.
        //! Only slices spawned by this component are returned.
        virtual AZStd::vector<AzFramework::SliceInstantiationTicket> GetCurrentlySpawnedSlices() = 0;

        //! Returns whether this component has spawned any slices that haven't been destroyed.
        //! A slice is considered destroyed once all its entities are destroyed.
        //! Returns true if any slices haven't finished spawning yet.
        virtual bool HasAnyCurrentlySpawnedSlices() = 0;

        //! Returns the IDs of current entities from a spawned slice.
        //! Note that spawning is not instant, if the slice hasn't finished spawning then no entities are returned.
        //! If an entity has been destroyed since it was spawned, its ID is not returned.
        //! Only slices spawned by this component can be queried.
        virtual AZStd::vector<AZ::EntityId> GetCurrentEntitiesFromSpawnedSlice(const AzFramework::SliceInstantiationTicket& ticket) = 0;

        //! Returns the IDs of all existing entities spawned by this component.
        //! Note that spawning is not instant, if a slice hasn't finished spawning then none of its entities are returned.
        //! If an entity has been destroyed since it was spawned, its ID is not returned.
        virtual AZStd::vector<AZ::EntityId> GetAllCurrentlySpawnedEntities() = 0;

        //! Returns whether or not the spawner is in a state that's ready to spawn.
        virtual bool IsReadyToSpawn() = 0;
    };

    using SpawnerComponentRequestBus = AZ::EBus<SpawnerComponentRequests>;


    /*!
     * SpawnerComponentNotificationBus
     * Events dispatched by the SpawnerComponent
     */
    class SpawnerComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~SpawnerComponentNotifications() {}

        //! Notify that slice has been spawned, but entities have not yet been activated.
        //! OnEntitySpawned events are about to be dispatched.
        virtual void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

        //! Notify that a spawn has been completed. All OnEntitySpawned events have been dispatched.
        virtual void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

        //! Notify that an entity has spawned, will be called once for each entity spawned in a slice.
        virtual void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& /*ticket*/, const AZ::EntityId& /*spawnedEntity*/) {}

        //! Single event notification for an entire slice spawn, providing a list of all resulting entity Ids.
        virtual void OnEntitiesSpawned(const AzFramework::SliceInstantiationTicket& /*ticket*/, const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

        //! Notify of a spawned slice's destruction.
        //! This occurs when all entities from a spawn are destroyed, or the slice fails to spawn.
        virtual void OnSpawnedSliceDestroyed(const AzFramework::SliceInstantiationTicket& /*ticket*/) {}
    };

    using SpawnerComponentNotificationBus = AZ::EBus<SpawnerComponentNotifications>;
    
    class SpawnerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(SpawnerConfig, "{D4D68E8E-9031-448F-9D56-B5575CF4833C}", AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(SpawnerConfig, AZ::SystemAllocator);

        SpawnerConfig() = default;

        ///The slice asset to be spawned
        AZ::Data::Asset<AZ::DynamicSliceAsset> m_sliceAsset{ AZ::Data::AssetLoadBehavior::PreLoad };
        ///Whether or not to spawn the slice when the component activates
        bool m_spawnOnActivate = false;
        ///Whether or not to destroy the slice when the component deactivates
        bool m_destroyOnDeactivate = false;
    };

} // namespace LmbrCentral
