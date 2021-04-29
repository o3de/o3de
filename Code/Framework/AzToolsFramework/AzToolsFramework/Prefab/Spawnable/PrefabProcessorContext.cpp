/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Spawnable/Spawnable.h>

#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    PrefabProcessorContext::PrefabProcessorContext(const AZ::Uuid& sourceUuid)
        : m_sourceUuid(sourceUuid)
    {}

    bool PrefabProcessorContext::AddPrefab(AZStd::string prefabName, PrefabDom prefab)
    {
        auto result = m_prefabs.emplace(AZStd::move(prefabName), AZStd::move(prefab));
        return result.second;
    }

    void PrefabProcessorContext::ListPrefabs(const AZStd::function<void(AZStd::string_view, PrefabDom&)>& callback)
    {
        m_isIterating = true;
        for (auto& it : m_prefabs)
        {
            callback(it.first, it.second);
        }
        m_isIterating = false;
    }

    void PrefabProcessorContext::ListPrefabs(const AZStd::function<void(AZStd::string_view, const PrefabDom&)>& callback) const
    {
        for (const auto& it : m_prefabs)
        {
            callback(it.first, it.second);
        }
    }

    bool PrefabProcessorContext::HasPrefabs() const
    {
        return !m_prefabs.empty();
    }

    bool PrefabProcessorContext::RegisterProductDependency(const AZStd::string& prefabName, const AZStd::string& dependentPrefabName)
    {
        using ConversionUtils = PrefabConversionUtils::ProcessedObjectStore;

        uint32_t spawnableSubId = ConversionUtils::BuildSubId(prefabName +
            AzFramework::Spawnable::DotFileExtension);

        uint32_t spawnablePrefabSubId = ConversionUtils::BuildSubId(dependentPrefabName +
            AzFramework::Spawnable::DotFileExtension);

        return RegisterProductDependency(spawnableSubId, spawnablePrefabSubId);
    }

    bool PrefabProcessorContext::RegisterProductDependency(const AZStd::string& prefabName, const AZ::Data::AssetId& dependentAssetId)
    {
        using ConversionUtils = PrefabConversionUtils::ProcessedObjectStore;

        uint32_t spawnableSubId = ConversionUtils::BuildSubId(prefabName +
            AzFramework::Spawnable::DotFileExtension);

        AZ::Data::AssetId spawnableAssetId(GetSourceUuid(), spawnableSubId);

        return RegisterProductDependency(spawnableAssetId, dependentAssetId);
    }

    bool PrefabProcessorContext::RegisterProductDependency(uint32_t spawnableAssetSubId, uint32_t dependentSpawnableAssetSubId)
    {
        AZ::Data::AssetId spawnableAssetId(GetSourceUuid(), spawnableAssetSubId);
        AZ::Data::AssetId dependentSpawnableAssetId(GetSourceUuid(), dependentSpawnableAssetSubId);

        return RegisterProductDependency(spawnableAssetId, dependentSpawnableAssetId);
    }

    bool PrefabProcessorContext::RegisterProductDependency(const AZ::Data::AssetId& assetId, const AZ::Data::AssetId& dependentAssetId)
    {
        return m_registeredProductDependencies[assetId].emplace(dependentAssetId).second;
    }

    PrefabProcessorContext::ProcessedObjectStoreContainer& PrefabProcessorContext::GetProcessedObjects()
    {
        return m_products;
    }

    const PrefabProcessorContext::ProcessedObjectStoreContainer& PrefabProcessorContext::GetProcessedObjects() const
    {
        return m_products;
    }

    PrefabProcessorContext::ProductDependencyContainer& PrefabProcessorContext::GetRegisteredProductDependencies()
    {
        return m_registeredProductDependencies;
    }

    const PrefabProcessorContext::ProductDependencyContainer& PrefabProcessorContext::GetRegisteredProductDependencies() const
    {
        return m_registeredProductDependencies;
    }

    void PrefabProcessorContext::SetPlatformTags(AZ::PlatformTagSet tags)
    {
        m_platformTags = AZStd::move(tags);
    }

    const AZ::PlatformTagSet& PrefabProcessorContext::GetPlatformTags() const
    {
        return m_platformTags;
    }

    const AZ::Uuid& PrefabProcessorContext::GetSourceUuid() const
    {
        return m_sourceUuid;
    }

    bool PrefabProcessorContext::HasCompletedSuccessfully() const
    {
        return m_completedSuccessfully;
    }

    void PrefabProcessorContext::ErrorEncountered()
    {
        m_completedSuccessfully = false;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
