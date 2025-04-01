/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentExport.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/EntityAliasTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/EntityIdPathMapperInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabDocument.h>
#include <AzToolsFramework/Prefab/Spawnable/ProcesedObjectStore.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    struct AssetDependencyInfo
    {
        AZ::Data::AssetId m_assetId;
        AZ::Data::AssetLoadBehavior m_loadBehavior;
    };

    using PrefabSpawnablePostProcessEvent = AZ::Event<const AZStd::string&, AzFramework::Spawnable&>;

    class PrefabProcessorContext : private EntityIdPathMapperInterface
    {
    public:
        using ProcessedObjectStoreContainer = AZStd::vector<ProcessedObjectStore>;
        using ProductAssetDependencyContainer = AZStd::unordered_multimap<AZ::Data::AssetId, AssetDependencyInfo>;

        AZ_CLASS_ALLOCATOR(PrefabProcessorContext, AZ::SystemAllocator);
        AZ_RTTI(PrefabProcessorContext, "{C7D77E3A-C544-486B-B774-7C82C38FE22F}");

        explicit PrefabProcessorContext(const AZ::Uuid& sourceUuid);
        virtual ~PrefabProcessorContext();

        virtual bool AddPrefab(PrefabDocument&& document);
        virtual void ListPrefabs(const AZStd::function<void(PrefabDocument&)>& callback);
        virtual void ListPrefabs(const AZStd::function<void(const PrefabDocument&)>& callback) const;
        virtual bool HasPrefabs() const;

        virtual bool RegisterSpawnableProductAssetDependency(
            AZStd::string prefabName, AZStd::string dependentPrefabName, EntityAliasSpawnableLoadBehavior loadBehavior);
        virtual bool RegisterSpawnableProductAssetDependency(
            AZStd::string prefabName, const AZ::Data::AssetId& dependentAssetId, EntityAliasSpawnableLoadBehavior loadBehavior);
        virtual bool RegisterSpawnableProductAssetDependency(
            uint32_t spawnableAssetSubId, uint32_t dependentSpawnableAssetSubId, EntityAliasSpawnableLoadBehavior loadBehavior);
        virtual bool RegisterProductAssetDependency(const AZ::Data::AssetId& assetId, const AZ::Data::AssetId& dependentAssetId);
        virtual bool RegisterProductAssetDependency(
            const AZ::Data::AssetId& assetId, const AZ::Data::AssetId& dependentAssetId, AZ::Data::AssetLoadBehavior loadBehavior);

        virtual void RegisterSpawnableEntityAlias(EntityAliasStore link);
        virtual void ResolveSpawnableEntityAliases(
            AZStd::string_view prefabName, AzFramework::Spawnable& spawnable, const AzToolsFramework::Prefab::Instance& instance);
        
        virtual ProcessedObjectStoreContainer& GetProcessedObjects();
        virtual const ProcessedObjectStoreContainer& GetProcessedObjects() const;

        virtual ProductAssetDependencyContainer& GetRegisteredProductAssetDependencies();
        virtual const ProductAssetDependencyContainer& GetRegisteredProductAssetDependencies() const;

        virtual void SetPlatformTags(AZ::PlatformTagSet tags);
        virtual const AZ::PlatformTagSet& GetPlatformTags() const;
        virtual const AZ::Uuid& GetSourceUuid() const;

        virtual void ResolveLinks();

        virtual bool HasCompletedSuccessfully() const;
        virtual void ErrorEncountered();

        //! EntityIdPathMapperInterface overrides
        AZ::IO::PathView GetHashedPathUsedForEntityIdGeneration(const AZ::EntityId) override;
        void SetHashedPathUsedForEntityIdGeneration(const AZ::EntityId, AZ::IO::PathView) override;

        void AddPrefabSpawnablePostProcessEventHandler(PrefabSpawnablePostProcessEvent::Handler& handler);
        void SendSpawnablePostProcessEvent(const AZStd::string& prefabName, AzFramework::Spawnable& spawnable);

    protected:
        using PrefabNames = AZStd::unordered_set<AZStd::string>;
        using PrefabContainer = AZStd::vector<PrefabDocument>;
        using SpawnableEntityAliasStore = AZStd::vector<EntityAliasStore>;

        AZ::Data::AssetLoadBehavior ToAssetLoadBehavior(EntityAliasSpawnableLoadBehavior loadBehavior) const;

        AZStd::unordered_map<AZ::EntityId, AZ::IO::Path> m_entityIdToHashedPathMap;
        PrefabContainer m_prefabs;
        PrefabContainer m_pendingPrefabAdditions;
        PrefabNames m_prefabNames;
        SpawnableEntityAliasStore m_entityAliases;
        ProcessedObjectStoreContainer m_products;
        ProductAssetDependencyContainer m_registeredProductAssetDependencies;
        PrefabSpawnablePostProcessEvent m_prefabPostProcessEvent;

        AZ::PlatformTagSet m_platformTags;
        AZ::Uuid m_sourceUuid;
        bool m_isIterating{ false };
        bool m_completedSuccessfully{ true };
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
