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
#include <AzFramework/Spawnable/InMemorySpawnableAssetContainer.h>

namespace AzToolsFramework::Prefab
{
    class PrefabSystemComponentInterface;
    class PrefabLoaderInterface;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabInMemorySpawnableConverter
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabInMemorySpawnableConverter, AZ::SystemAllocator);

        ~PrefabInMemorySpawnableConverter();

        bool Activate(AZStd::string_view stackProfile);
        void Deactivate();
        bool IsActivated() const;
        AZStd::string_view GetStackProfile() const;

        //! Converts a prefab file into an in-memory spawnable asset.
        AzFramework::InMemorySpawnableAssetContainer::CreateSpawnableResult CreateInMemorySpawnableAsset(
            AZStd::string_view prefabFilePath, AZStd::string_view spawnableName, bool loadReferencedAssets = false);

        //! Converts a prefab template into an in-memory spawnable asset.
        //! Example use case: convert an unsaved prefab into a spawnable asset to be spawned in editor game mode
        AzFramework::InMemorySpawnableAssetContainer::CreateSpawnableResult CreateInMemorySpawnableAsset(
            AzToolsFramework::Prefab::TemplateId templateId, AZStd::string_view spawnableName, bool loadReferencedAssets = false);

        const AzFramework::InMemorySpawnableAssetContainer& GetAssetContainerConst() const;
        AzFramework::InMemorySpawnableAssetContainer& GetAssetContainer();

    private:
        AzFramework::InMemorySpawnableAssetContainer m_assetContainer;
        PrefabConversionUtils::PrefabConversionPipeline m_converter;
        AZStd::string_view m_stackProfile;
        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        PrefabLoaderInterface* m_loaderInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
