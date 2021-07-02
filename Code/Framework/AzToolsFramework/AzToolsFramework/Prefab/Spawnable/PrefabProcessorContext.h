/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/ProcesedObjectStore.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext
    {
    public:
        using ProcessedObjectStoreContainer = AZStd::vector<ProcessedObjectStore>;
        using ProductAssetDependencyContainer =
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::unordered_set<AZ::Data::AssetId>>;

        AZ_CLASS_ALLOCATOR(PrefabProcessorContext, AZ::SystemAllocator, 0);
        AZ_RTTI(PrefabProcessorContext, "{C7D77E3A-C544-486B-B774-7C82C38FE22F}");

        explicit PrefabProcessorContext(const AZ::Uuid& sourceUuid);
        virtual ~PrefabProcessorContext() = default;

        virtual bool AddPrefab(AZStd::string prefabName, PrefabDom prefab);
        virtual void ListPrefabs(const AZStd::function<void(AZStd::string_view, PrefabDom&)>& callback);
        virtual void ListPrefabs(const AZStd::function<void(AZStd::string_view, const PrefabDom&)>& callback) const;
        virtual bool HasPrefabs() const;

        virtual bool RegisterSpawnableProductAssetDependency(AZStd::string prefabName, AZStd::string dependentPrefabName);
        virtual bool RegisterSpawnableProductAssetDependency(AZStd::string prefabName, const AZ::Data::AssetId& dependentAssetId);
        virtual bool RegisterSpawnableProductAssetDependency(uint32_t spawnableAssetSubId, uint32_t dependentSpawnableAssetSubId);
        virtual bool RegisterProductAssetDependency(const AZ::Data::AssetId& assetId, const AZ::Data::AssetId& dependentAssetId);

        virtual ProcessedObjectStoreContainer& GetProcessedObjects();
        virtual const ProcessedObjectStoreContainer& GetProcessedObjects() const;

        virtual ProductAssetDependencyContainer& GetRegisteredProductAssetDependencies();
        virtual const ProductAssetDependencyContainer& GetRegisteredProductAssetDependencies() const;

        virtual void SetPlatformTags(AZ::PlatformTagSet tags);
        virtual const AZ::PlatformTagSet& GetPlatformTags() const;
        virtual const AZ::Uuid& GetSourceUuid() const;

        virtual bool HasCompletedSuccessfully() const;
        virtual void ErrorEncountered();

    protected:
        using NamedPrefabContainer = AZStd::unordered_map<AZStd::string, PrefabDom>;

        NamedPrefabContainer m_prefabs;
        ProcessedObjectStoreContainer m_products;
        ProductAssetDependencyContainer m_registeredProductAssetDependencies;

        AZ::PlatformTagSet m_platformTags;
        AZ::Uuid m_sourceUuid;
        bool m_isIterating{ false };
        bool m_completedSuccessfully{ true };
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
