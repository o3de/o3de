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
#include <AzToolsFramework/Prefab/Spawnable/ProcesedObjectStore.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    enum class EntityAliasType : uint8_t
    {
        Disable,         //!< No alias is added.
        OptionalReplace, //!< At runtime the entity might be replaced. If the alias is disabled the original entity will be spawned.
                         //!<   The original entity will be left in the spawnable and a copy is returned.
        Replace,         //!< At runtime the entity will be replaced. If the alias is disabled nothing will be spawned. The original
                         //!<   entity is returned and a blank entity is left.
        Additional,      //!< At runtime the alias entity will be added as an additional but unrelated entity with a new entity id.
                         //!<   An empty entity will be returned.
        Merge            //!< At runtime the components in both entities will be merged. An empty entity will be returned. The added
                         //!<   components may no conflict with the entities already in the root entity.
    };

    enum class EntityAliasSpawnableLoadBehavior : uint8_t
    {
        NoLoad,         //!< Don't load the spawnable referenced in the entity alias. Loading will be up to the caller.
        QueueLoad,      //!< Queue the spawnable referenced in the entity alias for loading. This will be an async load because asset
                        //!<    handlers aren't allowed to start a blocking load as this can lead to deadlocks. This option will allow
                        //!<    to disable loading the referenced spawnable through the event fired from the spawnables asset handler.
        DependentLoad   //!< The spawnable referenced in the entity alias is made a dependency of the spawnable that holds the entity
                        //!<    alias. This will cause the spawnable to be automatically loaded along with the owning spawnable.
    };

    struct EntityAliasSpawnableLink
    {
        EntityAliasSpawnableLink(AzFramework::Spawnable& spawnable, AZ::EntityId index);

        AzFramework::Spawnable& m_spawnable;
        AZ::EntityId m_index;
    };

    struct EntityAliasPrefabLink
    {
        EntityAliasPrefabLink(AZStd::string prefabName, AzToolsFramework::Prefab::AliasPath alias);

        AZStd::string m_prefabName;
        AzToolsFramework::Prefab::AliasPath m_alias;
    };

    struct EntityAliasStore
    {
        using LinkStore = AZStd::variant<AZStd::monostate, EntityAliasSpawnableLink, EntityAliasPrefabLink>;

        LinkStore m_source;
        LinkStore m_target;
        uint32_t m_tag;
        AzFramework::Spawnable::EntityAliasType m_aliasType;
        EntityAliasSpawnableLoadBehavior m_loadBehavior;
    };

    struct AssetDependencyInfo
    {
        AZ::Data::AssetId m_assetId;
        AZ::Data::AssetLoadBehavior m_loadBehavior;
    };

    class PrefabProcessorContext
    {
    public:
        using ProcessedObjectStoreContainer = AZStd::vector<ProcessedObjectStore>;
        using ProductAssetDependencyContainer = AZStd::unordered_multimap<AZ::Data::AssetId, AssetDependencyInfo>;

        AZ_CLASS_ALLOCATOR(PrefabProcessorContext, AZ::SystemAllocator, 0);
        AZ_RTTI(PrefabProcessorContext, "{C7D77E3A-C544-486B-B774-7C82C38FE22F}");

        explicit PrefabProcessorContext(const AZ::Uuid& sourceUuid);
        virtual ~PrefabProcessorContext() = default;

        virtual bool AddPrefab(AZStd::string prefabName, PrefabDom prefab);
        virtual void ListPrefabs(const AZStd::function<void(AZStd::string_view, PrefabDom&)>& callback);
        virtual void ListPrefabs(const AZStd::function<void(AZStd::string_view, const PrefabDom&)>& callback) const;
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

    protected:
        using NamedPrefabContainer = AZStd::unordered_map<AZStd::string, PrefabDom>;
        using SpawnableEntityAliasStore = AZStd::vector<EntityAliasStore>;

        AZ::Data::AssetLoadBehavior ToAssetLoadBehavior(EntityAliasSpawnableLoadBehavior loadBehavior) const;

        NamedPrefabContainer m_prefabs;
        SpawnableEntityAliasStore m_entityAliases;
        ProcessedObjectStoreContainer m_products;
        ProductAssetDependencyContainer m_registeredProductAssetDependencies;

        AZ::PlatformTagSet m_platformTags;
        AZ::Uuid m_sourceUuid;
        bool m_isIterating{ false };
        bool m_completedSuccessfully{ true };
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
