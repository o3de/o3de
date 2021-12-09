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
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>

namespace AzToolsFramework::Prefab
{
    class PrefabSystemComponentInterface;
    class PrefabLoaderInterface;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{

    class InMemorySpawnableAssetContainer
    {
    public:
        AZ_CLASS_ALLOCATOR(InMemorySpawnableAssetContainer, AZ::SystemAllocator, 0);

        using Assets = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
        using CreateSpawnableResult = AZ::Outcome<AZ::Data::Asset<AZ::Data::AssetData>&, AZStd::string>;
        using RemoveSpawnableResult = AZ::Outcome<void, AZStd::string>;

        struct SpawnableAssetData
        {
            Assets m_assets;
            AZ::Data::AssetId m_spawnableAssetId;
        };
        using SpawnableAssets = AZStd::unordered_map<AZStd::string, SpawnableAssetData>;

        virtual ~InMemorySpawnableAssetContainer();

        bool Activate(AZStd::string_view stackProfile);
        void Deactivate();
        bool IsActivated() const;
        AZStd::string_view GetStockProfile() const;

        CreateSpawnableResult CreateInMemorySpawnableAsset(
            AZStd::string_view prefabFilePath, AZStd::string_view spawnableName, bool loadReferencedAssets = false);
        CreateSpawnableResult CreateInMemorySpawnableAsset(
            AzToolsFramework::Prefab::TemplateId templateId, AZStd::string_view spawnableName, bool loadReferencedAssets = false);
        RemoveSpawnableResult RemoveInMemorySpawnableAsset(AZStd::string_view spawnableName);
        AZ::Data::AssetId GetInMemorySpawnableAssetId(AZStd::string_view spawnableName) const;
        bool HasInMemorySpawnableAsset(AZStd::string_view spawnableName) const;

        void ClearAllInMemorySpawnableAssets();
        SpawnableAssets&& MoveAllInMemorySpawnableAssets();
        const SpawnableAssets& GetAllInMemorySpawnableAssets() const;

    private:
        void LoadReferencedAssets(AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets);

        SpawnableAssets m_spawnableAssets;
        PrefabConversionUtils::PrefabConversionPipeline m_converter;
        AZStd::string_view m_stockProfile;
        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        PrefabLoaderInterface* m_loaderInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
