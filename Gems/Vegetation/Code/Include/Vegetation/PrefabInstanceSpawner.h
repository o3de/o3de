/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Vegetation/InstanceSpawner.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace Vegetation
{
    /**
    * Instance spawner of prefab instances.
    */
    class PrefabInstanceSpawner
        : public InstanceSpawner
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_RTTI(PrefabInstanceSpawner, "{74BEEDB5-81CF-409F-B375-0D93D81EF2E3}", InstanceSpawner);
        AZ_CLASS_ALLOCATOR(PrefabInstanceSpawner, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        PrefabInstanceSpawner();
        virtual ~PrefabInstanceSpawner();

        //! Start loading any assets that the spawner will need.
        void LoadAssets() override;

        //! Unload any assets that the spawner loaded.
        void UnloadAssets() override;

        //! Perform any extra initialization needed at the point of registering with the vegetation system.
        void OnRegisterUniqueDescriptor() override;

        //! Perform any extra cleanup needed at the point of unregistering with the vegetation system.
        void OnReleaseUniqueDescriptor() override;

        //! Does this exist but have empty asset references?
        bool HasEmptyAssetReferences() const override;

        //! Has this finished loading any assets that are needed?
        bool IsLoaded() const override;

        //! Are the assets loaded, initialized, and spawnable?
        bool IsSpawnable() const override;

        //! Display name of the instances that will be spawned.
        AZStd::string GetName() const override;

        //! Create a single instance.
        InstancePtr CreateInstance(const InstanceData& instanceData) override;

        //! Destroy a single instance.
        void DestroyInstance(InstanceId id, InstancePtr instance) override;

        AZStd::string GetSpawnableAssetPath() const;
        void SetSpawnableAssetPath(const AZStd::string& assetPath);

        AZ::Data::AssetId GetSpawnableAssetId() const;
        void SetSpawnableAssetId(const AZ::Data::AssetId& assetId);

    private:
        bool DataIsEquivalent(const InstanceSpawner& rhs) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::u32 SpawnableAssetChanged();
        void ResetSpawnableAsset();

        void UpdateCachedValues();

        //! Verify that the spawnable asset only contains data compatible with the dynamic vegetation system.
        bool ValidateAssetContents(const AZ::Data::Asset<AZ::Data::AssetData> asset) const;

        //! Despawn an instance of a spawnable asset
        void DespawnAssetInstance(AzFramework::EntitySpawnTicket* ticket);

        //! Cached values so that asset isn't accessed on other threads
        bool m_assetLoadedAndSpawnable = false;

        //! Collection of spawned instance tickets, needed for destroying the instances.
        AZStd::unordered_set<AzFramework::EntitySpawnTicket*> m_instanceTickets;

        //! asset data
        AZ::Data::Asset<AzFramework::Spawnable> m_spawnableAsset;
    };

} // namespace Vegetation
