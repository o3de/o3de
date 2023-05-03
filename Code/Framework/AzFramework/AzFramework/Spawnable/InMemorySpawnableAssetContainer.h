/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AzFramework
{
    class Spawnable;

    class InMemorySpawnableAssetContainer
    {
    public:
        AZ_CLASS_ALLOCATOR(InMemorySpawnableAssetContainer, AZ::SystemAllocator);

        using Assets = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
        using CreateSpawnableResult = AZ::Outcome<AZStd::reference_wrapper<AZ::Data::Asset<AZ::Data::AssetData>>, AZStd::string>;
        using RemoveSpawnableResult = AZ::Outcome<void, AZStd::string>;

        using AssetDataInfoPair = AZStd::pair<AZ::Data::AssetData*, AZ::Data::AssetInfo>;
        using AssetDataInfoContainer = AZStd::vector<AssetDataInfoPair>;

        struct SpawnableAssetData
        {
            Assets m_assets;
            AZ::Data::AssetId m_spawnableAssetId;
        };
        using SpawnableAssets = AZStd::unordered_map<AZStd::string, SpawnableAssetData>;

        ~InMemorySpawnableAssetContainer() = default;

        RemoveSpawnableResult RemoveInMemorySpawnableAsset(AZStd::string_view spawnableName);
        AZ::Data::AssetId GetInMemorySpawnableAssetId(AZStd::string_view spawnableName) const;
        bool HasInMemorySpawnableAsset(AZStd::string_view spawnableName) const;

        void ClearAllInMemorySpawnableAssets();
        SpawnableAssets&& MoveAllInMemorySpawnableAssets();
        const SpawnableAssets& GetAllInMemorySpawnableAssets() const;

        //! Creates an in-memory spawnable asset given a list of product asset data
        //! @param assetDataInfoContainer a list of asset data/info pairs to be converted into a spawnable asset
        //! @param loadReferencedAssets a boolean that indicates whether to load the assets referenced from the asset data
        //! @param targetSpawnableName the name of the target spawnable whose asset should be returned upon success
        //! @return an outcome containing the target spawnable asset upon success
        CreateSpawnableResult CreateInMemorySpawnableAsset(
            AssetDataInfoContainer& assetDataInfoContainer,
            bool loadReferencedAssets,
            const AZStd::string& targetSpawnableName);

        //! Creates an in-memory spawnable asset given a single spawnable asset data.
        //! Ex. a network server receives in-memory spawnable data from client and creates a spawnable asset
        //! @param spawnable the spawnable asset data to be converted into a spawnable asset
        //! @param assetId the Id of the new asset
        //! @param assetSize the size of the new asset
        //! @param loadReferencedAssets a boolean that indicates whether to load the assets referenced from the asset data
        //! @param targetSpawnableName the name of the target spawnable whose asset should be returned upon success
        //! @return an outcome containing the target spawnable asset upon success
        CreateSpawnableResult CreateInMemorySpawnableAsset(
            Spawnable* spawnable,
            const AZ::Data::AssetId& assetId,
            bool loadReferencedAssets,
            const AZStd::string& targetSpawnableName);

    private:
        void LoadReferencedAssets(SpawnableAssetData& spawnable);

        SpawnableAssets m_spawnableAssets;
    };
} // namespace AzFramework
