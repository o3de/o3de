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
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/InMemorySpawnableAssetContainer.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConverterStackProfileNames.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    InMemorySpawnableAssetContainer::~InMemorySpawnableAssetContainer()
    {
        Deactivate();
    }

    bool InMemorySpawnableAssetContainer::Activate(AZStd::string_view stackProfile)
    {
        AZ_Assert(!IsActivated(),
            "InMemorySpawnableAssetContainer - Unable to activate an instance of InMemorySpawnableAssetContainer as the instance is already active.");

        m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponentInterface, "InMemorySpawnableAssetContainer - Could not retrieve instance of PrefabSystemComponentInterface");

        m_loaderInterface = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_loaderInterface, "InMemorySpawnableAssetContainer - Could not retrieve instance of PrefabLoaderInterface");

        m_stockProfile = stackProfile;
        return m_converter.LoadStackProfile(m_stockProfile);
    }

    void InMemorySpawnableAssetContainer::Deactivate()
    {
        ClearAllInMemorySpawnableAssets();
        m_stockProfile = "";
        m_loaderInterface = nullptr;
        m_prefabSystemComponentInterface = nullptr;
    }

    bool InMemorySpawnableAssetContainer::IsActivated() const
    {
        return m_converter.IsLoaded();
    }

    AZStd::string_view InMemorySpawnableAssetContainer::GetStockProfile() const
    {
        return m_stockProfile;
    }

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
        AzToolsFramework::Prefab::TemplateId templateId, AZStd::string_view spawnableName, bool loadReferencedAssets)
    {
        if (!IsActivated())
        {
            return AZ::Failure(AZStd::string::format("Failed to create a prefab processing stack from key '%.*s'.", AZ_STRING_ARG(m_stockProfile)));
        }

        if (HasInMemorySpawnableAsset(spawnableName))
        {
            return AZ::Failure(AZStd::string::format("In-memory Spawnable '%.*s' already exists.", AZ_STRING_ARG(spawnableName)));
        }

        TemplateReference templateReference = m_prefabSystemComponentInterface->FindTemplate(templateId);
        if (!templateReference.has_value())
        {
            return AZ::Failure(AZStd::string::format("Could not get Template DOM for given Template's id %llu .", templateId));
        }

        // Use a random uuid as this is only a temporary source.
        PrefabConversionUtils::PrefabProcessorContext context(AZ::Uuid::CreateRandom());
        PrefabDocument document(spawnableName);
        document.SetPrefabDom(templateReference->get().GetPrefabDom());
        context.AddPrefab(AZStd::move(document));
        m_converter.ProcessPrefab(context);

        if (!context.HasCompletedSuccessfully() || context.GetProcessedObjects().empty())
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to convert the prefab into assets. Please confirm that the '%.*s' prefab processor stack is capable of producing a usable product asset.",
                AZ_STRING_ARG(PrefabConversionUtils::IntegrationTests)));
        }

        static constexpr size_t NoTargetSpawnable = AZStd::numeric_limits<size_t>::max();
        size_t targetSpawnableIndex = NoTargetSpawnable;
        AZStd::vector<AZ::Data::AssetId> assetIds;
        SpawnableAssetData spawnableAssetData;
        AZStd::string rootProductId(spawnableName);
        rootProductId += AzFramework::Spawnable::DotFileExtension;

        // Create temporary assets from the processed data.
        for (auto& product : context.GetProcessedObjects())
        {
            if (product.GetAssetType() == azrtti_typeid<AzFramework::Spawnable>() && product.GetId() == rootProductId)
            {
                targetSpawnableIndex = spawnableAssetData.m_assets.size();
            }

            AZ::Data::AssetInfo info;
            info.m_assetId = product.GetAsset().GetId();
            info.m_assetType = product.GetAssetType();
            info.m_relativePath = product.GetId();

            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, info.m_assetId, info);
            spawnableAssetData.m_assets.emplace_back(product.ReleaseAsset().release(), AZ::Data::AssetLoadBehavior::Default);

            // Ensure the product asset is registered with the AssetManager
            // Hold on to the returned asset to keep ref count alive until we assign it the latest data
            AZ::Data::Asset<AZ::Data::AssetData> asset =
                AZ::Data::AssetManager::Instance().FindOrCreateAsset(info.m_assetId, info.m_assetType, AZ::Data::AssetLoadBehavior::Default);

            // Update the asset registered in the AssetManager with the data of our product from the Prefab Processor
            AZ::Data::AssetManager::Instance().AssignAssetData(spawnableAssetData.m_assets.back());
        }

        if (targetSpawnableIndex == NoTargetSpawnable)
        {
            return AZ::Failure(AZStd::string::format("Failed to produce the target spawnable '%.*s'.", AZ_STRING_ARG(spawnableName)));
        }

        if (loadReferencedAssets)
        {
            LoadReferencedAssets(spawnableAssetData);
        }

        auto& spawnableAssetDataAdded = m_spawnableAssets.emplace(spawnableName, spawnableAssetData).first->second;
        spawnableAssetDataAdded.m_spawnableAssetId = spawnableAssetDataAdded.m_assets[targetSpawnableIndex].GetId();
        return AZ::Success(spawnableAssetDataAdded.m_assets[targetSpawnableIndex]);
    }

    InMemorySpawnableAssetContainer::CreateSpawnableResult InMemorySpawnableAssetContainer::CreateInMemorySpawnableAsset(
        AZStd::string_view prefabFilePath, AZStd::string_view spawnableName, bool loadReferencedAssets)
    {
        AZ::IO::Path relativePath = m_loaderInterface->GenerateRelativePath(prefabFilePath);
        auto templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);
        if (templateId == InvalidTemplateId)
        {
            return AZ::Failure(AZStd::string::format("Template with source path '%.*s' is not found.", AZ_STRING_ARG(prefabFilePath)));
        }

        return CreateInMemorySpawnableAsset(templateId, spawnableName, loadReferencedAssets);
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
            auto callback = [&blockingAssets](
                                void* object, const AZ::SerializeContext::ClassData* classData,
                                [[maybe_unused]]const AZ::SerializeContext::ClassElement* elementData) -> bool
            {
                if (classData->m_typeId == AZ::GetAssetClassId())
                {
                    auto asset = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(object);

                    if (!asset->GetId().IsValid())
                    {
                        // Invalid asset found referenced in scene while entering game mode.
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
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
