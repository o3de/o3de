/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Spawnable/InMemorySpawnableAssetContainer.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetUtils.h>

namespace AzFramework
{
    bool InMemorySpawnableAssetContainer::HasInMemorySpawnableAsset(AZStd::string_view spawnableName) const
    {
        return m_spawnableAssets.find(spawnableName) != m_spawnableAssets.end();
    }

    AZ::Data::AssetId InMemorySpawnableAssetContainer::GetInMemorySpawnableAssetId(AZStd::string_view spawnableName) const
    {
        auto found = m_spawnableAssets.find(spawnableName);
        if (found != m_spawnableAssets.end())
        {
            return found->second.m_spawnableAssetId;
        }
        else
        {
            return  AZ::Data::AssetId();
        }
    }

    auto InMemorySpawnableAssetContainer::RemoveInMemorySpawnableAsset(AZStd::string_view spawnableName) -> RemoveSpawnableResult
    {
        auto found = m_spawnableAssets.find(spawnableName);
        if (found == m_spawnableAssets.end())
        {
            return AZ::Failure(AZStd::string::format("In-memory Spawnable '%.*s' doesn't exists.", AZ_STRING_ARG(spawnableName)));
        }

        for (auto& asset : found->second.m_assets)
        {
            asset.Release();
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset.GetId());
        }

        m_spawnableAssets.erase(found);
        return AZ::Success();
    }

    InMemorySpawnableAssetContainer::CreateSpawnableResult InMemorySpawnableAssetContainer::CreateInMemorySpawnableAsset(
        AssetDataInfoContainer& assetDataInfoContainer,
        bool loadReferencedAssets,
        const AZStd::string& targetSpawnableName)
    {
        static constexpr size_t NoTargetSpawnable = AZStd::numeric_limits<size_t>::max();
        size_t targetSpawnableIndex = NoTargetSpawnable;
        SpawnableAssetData spawnableAssetData;
        AZStd::string rootProductId = targetSpawnableName + Spawnable::DotFileExtension;

        AZStd::vector<AZStd::pair<Spawnable*, const AZStd::string&>> spawnables;

        // Create temporary assets from the processed data.
        for (auto& product : assetDataInfoContainer)
        {
            AZ::Data::AssetData* assetData = product.first;
            AZ::Data::AssetInfo& assetInfo = product.second;

            if (assetInfo.m_assetType == azrtti_typeid<Spawnable>())
            {
                auto spawnable = azrtti_cast<Spawnable*>(assetData);

                spawnables.push_back(AZStd::pair<Spawnable*, const AZStd::string&>(spawnable, assetInfo.m_relativePath));

                if (assetInfo.m_relativePath == rootProductId)
                {
                    targetSpawnableIndex = spawnableAssetData.m_assets.size();
                }
            }

            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, assetInfo.m_assetId, assetInfo);

            // The asset Id may not already be set in the asset data in which case we will pass it to the asset creation.
            // For example, spawnable asset data is serialized and sent to the server, but the asset Id is not included
            // in the serialization
            if (assetData->GetId().IsValid())
            {
                AZ_Assert(assetData->GetId() == assetInfo.m_assetId, "Asset data already has an asset Id but it's different from the Id set in the corresponding assetInfo.");
                spawnableAssetData.m_assets.emplace_back(assetData, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                spawnableAssetData.m_assets.emplace_back(assetInfo.m_assetId, assetData, AZ::Data::AssetLoadBehavior::Default);
            }

            // Ensure the product asset is registered with the AssetManager
            // Hold on to the returned asset to keep ref count alive until we assign it the latest data
            AZ::Data::Asset<AZ::Data::AssetData> asset =
                AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetInfo.m_assetId, assetInfo.m_assetType, AZ::Data::AssetLoadBehavior::Default);

            // Update the asset registered in the AssetManager with the data of our product from the Prefab Processor
            AZ::Data::AssetManager::Instance().AssignAssetData(spawnableAssetData.m_assets.back());
        }

        if (targetSpawnableIndex == NoTargetSpawnable)
        {
            return AZ::Failure(AZStd::string::format("Failed to produce the target spawnable '%.*s'.", AZ_STRING_ARG(targetSpawnableName)));
        }

        if (loadReferencedAssets)
        {
            LoadReferencedAssets(spawnableAssetData);
        }

        // Delay resolving aliases to guarantee that the depended spawnables are already registered.
        for (auto spawnablePair : spawnables)
        {
            Spawnable* spawnable = spawnablePair.first;
            const AZStd::string& spawnableName = spawnablePair.second;
            SpawnableAssetUtils::ResolveEntityAliases(spawnable, spawnableName);
        }

        auto& spawnableAssetDataAdded = m_spawnableAssets.emplace(targetSpawnableName, spawnableAssetData).first->second;
        spawnableAssetDataAdded.m_spawnableAssetId = spawnableAssetDataAdded.m_assets[targetSpawnableIndex].GetId();
        return AZ::Success(spawnableAssetDataAdded.m_assets[targetSpawnableIndex]);
    }

    InMemorySpawnableAssetContainer::CreateSpawnableResult InMemorySpawnableAssetContainer::CreateInMemorySpawnableAsset(
        Spawnable* spawnable,
        const AZ::Data::AssetId& assetId,
        bool loadReferencedAssets,
        const AZStd::string& rootSpawnableName)
    {
        AZ::Data::AssetInfo assetInfo;
        assetInfo.m_assetId = assetId;
        assetInfo.m_assetType = spawnable->GetType();
        assetInfo.m_relativePath = rootSpawnableName;
        AZ::Data::AssetData* assetData = spawnable;
        InMemorySpawnableAssetContainer::AssetDataInfoContainer assetDataInfoContainer;
        assetDataInfoContainer.emplace_back(assetData, assetInfo);

        return CreateInMemorySpawnableAsset(assetDataInfoContainer, loadReferencedAssets, rootSpawnableName);
    }

    void InMemorySpawnableAssetContainer::ClearAllInMemorySpawnableAssets()
    {
        for (auto& [spawnableName, spawnableAssetData] : m_spawnableAssets)
        {
            for (auto& asset : spawnableAssetData.m_assets)
            {
                asset.Release();
                AZ::Data::AssetCatalogRequestBus::Broadcast(
                    &AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset.GetId());
            }
        }
        
        m_spawnableAssets.clear();
    }

    InMemorySpawnableAssetContainer::SpawnableAssets&& InMemorySpawnableAssetContainer::MoveAllInMemorySpawnableAssets()
    {
        return AZStd::move(m_spawnableAssets);
    }

    const InMemorySpawnableAssetContainer::SpawnableAssets& InMemorySpawnableAssetContainer::GetAllInMemorySpawnableAssets() const
    {
        return m_spawnableAssets;
    }

    void InMemorySpawnableAssetContainer::LoadReferencedAssets(SpawnableAssetData& spawnable)
    {
        // Get the referenced assets directly from the product. This is done for two reasons:
        //  1. Avoids calls to the Asset Manager for assets that are already loaded.
        //  2. Gets the exact asset to load to avoid issues with assets that don't reload.
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>*> blockingAssets;

        AZ::SerializeContext* sc = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(sc, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(sc, "Unable to locate Serialize Context while resolving asset references in the in-memory spawnable asset container.");

        for (AZ::Data::Asset<AZ::Data::AssetData>& asset : spawnable.m_assets)
        {
            auto callback = [sc, &blockingAssets](
                                void* object, const AZ::SerializeContext::ClassData* classData,
                                const AZ::SerializeContext::ClassElement* elementData) -> bool
            {
                if (const auto* genericInfo = elementData ? elementData->m_genericClassInfo : sc->FindGenericClassInfo(classData->m_typeId); genericInfo && genericInfo->GetGenericTypeId() == AZ::GetAssetClassId())
                {
                    auto asset = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(object);

                    if (!asset->GetId().IsValid())
                    {
                        // Invalid asset found referenced in scene while entering game mode.
                        return false;
                    }

                    if (asset->GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
                    {
                        AZ_Error(
                            "Prefab",
                            false,
                            "Asset '%s' (%s) of type '%s' failed to load while entering game mode",
                            asset->GetHint().c_str(),
                            asset->GetId().ToFixedString().c_str(),
                            classData->m_name);

                        return false;
                    }

                    if (asset->GetStatus() != AZ::Data::AssetData::AssetStatus::NotLoaded)
                    {
                        // Already loaded so no need to do anything.
                        return false;
                    }

                    const AZ::Data::AssetLoadBehavior loadBehavior = asset->GetAutoLoadBehavior();
                    if (loadBehavior == AZ::Data::AssetLoadBehavior::NoLoad)
                    {
                        return false;
                    }

                    if (loadBehavior == AZ::Data::AssetLoadBehavior::PreLoad)
                    {
                        // Only assets that are preloaded need to be waited on.
                        blockingAssets.push_back(asset);
                    }
                    if (!asset->QueueLoad())
                    {
                        AZ_Error(
                            "Prefab", false, "Failed to queue asset '%s' (%s) of type '%s' for loading while entering game mode.",
                            asset->GetHint().c_str(), asset->GetId().ToFixedString().c_str(),
                            asset->GetType().ToFixedString().c_str());
                        return false;
                    }

                    return false;
                }
                return true;
            };

            AZ::SerializeContext::EnumerateInstanceCallContext enumerationContext(
                callback, nullptr, sc, AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr);
            sc->EnumerateInstance(&enumerationContext, asset.GetData(), asset.GetType(), nullptr, nullptr);
        }

        // For all Preload assets we block until they're ready
        // We do this as a separate pass so that we don't interrupt queuing up all other asset loads
        for (AZ::Data::Asset<AZ::Data::AssetData>* asset : blockingAssets)
        {
            asset->BlockUntilLoadComplete();
            
            if (asset->IsError())
            {
                AZ_Error(
                    "Prefab", false, "Asset '%s' (%s) of type '%s' failed to preload while entering game mode", asset->GetHint().c_str(),
                    asset->GetId().ToFixedString().c_str(),
                    asset->GetType().ToFixedString().c_str());

                continue;
            }
        }
    }
} // namespace AzFramework
