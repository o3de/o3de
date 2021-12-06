/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/TempSpawnableAssetsCache.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    TempSpawnableAssetsCache::~TempSpawnableAssetsCache()
    {
        Deactivate();
    }

    bool TempSpawnableAssetsCache::Activate(AZStd::string_view stackProfile)
    {
        AZ_Assert(!IsActivated(), "TempSpawnableAssetsCache - TempSpawnableAssetsCache is currently activated");

        m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponentInterface, "TempSpawnableAssetsCache - Could not retrieve instance of PrefabSystemComponentInterface");

        m_loaderInterface = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_loaderInterface, "TempSpawnableAssetsCache - Could not retrieve instance of PrefabLoaderInterface");

        m_stockProfile = stackProfile;
        return m_converter.LoadStackProfile(m_stockProfile);
    }

    void TempSpawnableAssetsCache::Deactivate()
    {
        ClearAllTempSpawnableRelatedAssets();
        m_stockProfile = "";
        m_loaderInterface = nullptr;
        m_prefabSystemComponentInterface = nullptr;
    }

    bool TempSpawnableAssetsCache::IsActivated() const
    {
        return m_converter.IsLoaded();
    }

    AZStd::string_view TempSpawnableAssetsCache::GetStockProfile() const
    {
        return m_stockProfile;
    }

    TempSpawnableAssetsCache::CreateSpawnableResult TempSpawnableAssetsCache::CreateTempSpawnableAsset(
        AzToolsFramework::Prefab::TemplateId templateId, AZStd::string_view spawnableName, bool loadCreatedAssets)
    {
        if (!IsActivated())
        {
            return AZ::Failure(AZStd::string::format("Failed to create a prefab processing stack from key '%.*s'.", AZ_STRING_ARG(m_stockProfile)));
        }

        TemplateReference templateReference = m_prefabSystemComponentInterface->FindTemplate(templateId);
        if (!templateReference.has_value())
        {
            return AZ::Failure(AZStd::string::format("Could not get Template DOM given Template's id %llu .", templateId));
        }

        // Use a random uuid as this is only a temporary source.
        PrefabConversionUtils::PrefabProcessorContext context(AZ::Uuid::CreateRandom());
        PrefabDom copy;
        copy.CopyFrom(templateReference->get().GetPrefabDom(), copy.GetAllocator(), false);
        context.AddPrefab(spawnableName, AZStd::move(copy));
        m_converter.ProcessPrefab(context);

        if (!context.HasCompletedSuccessfully() || context.GetProcessedObjects().empty())
        {
            return AZ::Failure(AZStd::string(
                "Failed to convert the prefab into assets."
                "Please confirm that the 'TempSpawnables' prefab processor stack is capable of producing a usable product asset."));
        }

        static constexpr size_t NoTargetSpawnable = AZStd::numeric_limits<size_t>::max();
        size_t targetSpawnableIndex = NoTargetSpawnable;
        AZStd::vector<AZ::Data::AssetId> assetIds;

        // Create temporary assets from the processed data.
        for (auto& product : context.GetProcessedObjects())
        {
            if (product.GetAssetType() == AZ::AzTypeInfo<AzFramework::Spawnable>::Uuid() &&
                product.GetId() ==
                AZStd::string::format("%s.%s", spawnableName.data(), AzFramework::Spawnable::FileExtension))
            {
                targetSpawnableIndex = m_assets.size();
            }

            AZ::Data::AssetInfo info;
            info.m_assetId = product.GetAsset().GetId();
            info.m_assetType = product.GetAssetType();
            info.m_relativePath = product.GetId();

            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, info.m_assetId, info);
            m_assets.emplace_back(product.ReleaseAsset().release(), AZ::Data::AssetLoadBehavior::Default);

            // Ensure the product asset is registered with the AssetManager
            // Hold on to the returned asset to keep ref count alive until we assign it the latest data
            AZ::Data::Asset<AZ::Data::AssetData> asset =
                AZ::Data::AssetManager::Instance().FindOrCreateAsset(info.m_assetId, info.m_assetType, AZ::Data::AssetLoadBehavior::Default);

            // Update the asset registered in the AssetManager with the data of our product from the Prefab Processor
            AZ::Data::AssetManager::Instance().AssignAssetData(m_assets.back());
        }

        if (targetSpawnableIndex == NoTargetSpawnable)
        {
            return AZ::Failure(AZStd::string("Failed to produce the target spawnable '%.*s'", AZ_STRING_ARG(spawnableName)));
        }
        
        if (loadCreatedAssets)
        {
            for (auto& product : context.GetProcessedObjects())
            {
                LoadReferencedAssets(product.GetReferencedAssets());
            }
        }

        return AZ::Success(m_assets[targetSpawnableIndex]);
    }

    TempSpawnableAssetsCache::CreateSpawnableResult TempSpawnableAssetsCache::CreateTempSpawnableAsset(
        AZStd::string_view prefabFilePath, AZStd::string_view spawnableName, bool loadCreatedAssets)
    {
        AZ::IO::Path relativePath = m_loaderInterface->GenerateRelativePath(prefabFilePath);
        auto templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);
        if (templateId == InvalidTemplateId)
        {
            return AZ::Failure(AZStd::string::format("Template with source path '%.*s' is not found.", AZ_STRING_ARG(prefabFilePath)));
        }

        return CreateTempSpawnableAsset(templateId, spawnableName, loadCreatedAssets);
    }

    void TempSpawnableAssetsCache::ClearAllTempSpawnableRelatedAssets()
    {
        for (auto& asset : m_assets)
        {
            asset.Release();
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset.GetId());
        }

        m_assets.clear();
    }

    TempSpawnableAssetsCache::Assets&& TempSpawnableAssetsCache::MoveAllTempSpawnableRelatedAssets()
    {
        return AZStd::move(m_assets);
    }

    const TempSpawnableAssetsCache::Assets& TempSpawnableAssetsCache::GetAllTempSpawnableAsset() const
    {
        return m_assets;
    }

    void TempSpawnableAssetsCache::LoadReferencedAssets(AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets)
    {
        // Start our loads on all assets by calling GetAsset from the AssetManager
        for (AZ::Data::Asset<AZ::Data::AssetData>& asset : referencedAssets)
        {
            if (!asset.GetId().IsValid())
            {
                AZ_Error("Prefab", false, "Invalid asset found referenced in scene while entering game mode");
                continue;
            }

            const AZ::Data::AssetLoadBehavior loadBehavior = asset.GetAutoLoadBehavior();

            if (loadBehavior == AZ::Data::AssetLoadBehavior::NoLoad)
            {
                continue;
            }

            AZ::Data::AssetId assetId = asset.GetId();
            AZ::Data::AssetType assetType = asset.GetType();

            asset = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, loadBehavior);

            if (!asset.GetId().IsValid())
            {
                AZ_Error("Prefab", false, "Invalid asset found referenced in scene while entering game mode");
                continue;
            }
        }

        // For all Preload assets we block until they're ready
        // We do this as a seperate pass so that we don't interrupt queuing up all other asset loads
        for (AZ::Data::Asset<AZ::Data::AssetData>& asset : referencedAssets)
        {
            if (!asset.GetId().IsValid())
            {
                AZ_Error("Prefab", false, "Invalid asset found referenced in scene while entering game mode");
                continue;
            }

            const AZ::Data::AssetLoadBehavior loadBehavior = asset.GetAutoLoadBehavior();

            if (loadBehavior != AZ::Data::AssetLoadBehavior::PreLoad)
            {
                continue;
            }

            asset.BlockUntilLoadComplete();

            if (asset.IsError())
            {
                AZ_Error("Prefab", false, "Asset with id %s failed to preload while entering game mode",
                    asset.GetId().ToString<AZStd::string>().c_str());

                continue;
            }
        }
    }

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
