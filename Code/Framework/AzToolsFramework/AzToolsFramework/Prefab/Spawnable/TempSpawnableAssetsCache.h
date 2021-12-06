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

    class TempSpawnableAssetsCache
    {
    public:
        AZ_CLASS_ALLOCATOR(TempSpawnableAssetsCache, AZ::SystemAllocator, 0);

        using CreateSpawnableResult = AZ::Outcome<AZ::Data::Asset<AZ::Data::AssetData>&, AZStd::string>;
        using Assets = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;

        virtual ~TempSpawnableAssetsCache();

        bool Activate(AZStd::string_view stackProfile);
        void Deactivate();
        bool IsActivated() const;
        AZStd::string_view GetStockProfile() const;

        CreateSpawnableResult CreateTempSpawnableAsset(
            AZStd::string_view prefabFilePath, AZStd::string_view spawnableName, bool loadCreatedAssets = false);
        CreateSpawnableResult CreateTempSpawnableAsset(
            AzToolsFramework::Prefab::TemplateId templateId, AZStd::string_view spawnableName, bool loadCreatedAssets = false);
        void ClearAllTempSpawnableRelatedAssets();
        Assets&& MoveAllTempSpawnableRelatedAssets();
        const Assets& GetAllTempSpawnableAsset() const;

    private:
        void LoadReferencedAssets(AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets);

        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        PrefabLoaderInterface* m_loaderInterface = nullptr;
        PrefabConversionUtils::PrefabConversionPipeline m_converter;
        Assets m_assets;
        AZStd::string_view m_stockProfile;

    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
