/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    EntityAliasSpawnableLink::EntityAliasSpawnableLink(AzFramework::Spawnable& spawnable, AZ::EntityId index)
        : m_spawnable(spawnable)
        , m_index(index)
    {
    }

    EntityAliasPrefabLink::EntityAliasPrefabLink(AZStd::string prefabName, AzToolsFramework::Prefab::AliasPath alias)
        : m_prefabName(AZStd::move(prefabName))
        , m_alias(AZStd::move(alias))
    {
    }

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

    bool PrefabProcessorContext::RegisterSpawnableProductAssetDependency(
        AZStd::string prefabName, AZStd::string dependentPrefabName, EntityAliasSpawnableLoadBehavior loadBehavior)
    {
        using ConversionUtils = PrefabConversionUtils::ProcessedObjectStore;

        prefabName += AzFramework::Spawnable::DotFileExtension;
        uint32_t spawnableSubId = ConversionUtils::BuildSubId(AZStd::move(prefabName));

        dependentPrefabName += AzFramework::Spawnable::DotFileExtension;
        uint32_t spawnablePrefabSubId = ConversionUtils::BuildSubId(AZStd::move(dependentPrefabName));

        return RegisterSpawnableProductAssetDependency(spawnableSubId, spawnablePrefabSubId, loadBehavior);
    }

    bool PrefabProcessorContext::RegisterSpawnableProductAssetDependency(
        AZStd::string prefabName, const AZ::Data::AssetId& dependentAssetId, EntityAliasSpawnableLoadBehavior loadBehavior)
    {
        using ConversionUtils = PrefabConversionUtils::ProcessedObjectStore;

        prefabName += AzFramework::Spawnable::DotFileExtension;
        uint32_t spawnableSubId = ConversionUtils::BuildSubId(AZStd::move(prefabName));

        AZ::Data::AssetId spawnableAssetId(GetSourceUuid(), spawnableSubId);

        return RegisterProductAssetDependency(spawnableAssetId, dependentAssetId, ToAssetLoadBehavior(loadBehavior));
    }

    bool PrefabProcessorContext::RegisterSpawnableProductAssetDependency(
        uint32_t spawnableAssetSubId, uint32_t dependentSpawnableAssetSubId, EntityAliasSpawnableLoadBehavior loadBehavior)
    {
        AZ::Data::AssetId spawnableAssetId(GetSourceUuid(), spawnableAssetSubId);
        AZ::Data::AssetId dependentSpawnableAssetId(GetSourceUuid(), dependentSpawnableAssetSubId);

        return RegisterProductAssetDependency(spawnableAssetId, dependentSpawnableAssetId, ToAssetLoadBehavior(loadBehavior));
    }

    bool PrefabProcessorContext::RegisterProductAssetDependency(const AZ::Data::AssetId& assetId, const AZ::Data::AssetId& dependentAssetId)
    {
        return RegisterProductAssetDependency(assetId, dependentAssetId, AZ::Data::AssetLoadBehavior::NoLoad);
    }

    bool PrefabProcessorContext::RegisterProductAssetDependency(
        const AZ::Data::AssetId& assetId, const AZ::Data::AssetId& dependentAssetId, AZ::Data::AssetLoadBehavior loadBehavior)
    {
        auto dependencies = m_registeredProductAssetDependencies.equal_range(assetId);
        if (dependencies.first != dependencies.second)
        {
            for (auto it = dependencies.first; it != dependencies.second; ++it)
            {
                if (it->second.m_assetId == dependentAssetId)
                {
                    if (it->second.m_loadBehavior < loadBehavior)
                    {
                        it->second.m_loadBehavior = loadBehavior;
                    }
                    return true;
                }
            }
        }

        return m_registeredProductAssetDependencies.emplace(assetId, AssetDependencyInfo{ dependentAssetId, loadBehavior }).second;
    }

    void PrefabProcessorContext::RegisterSpawnableEntityAlias(EntityAliasStore link)
    {
        m_entityAliases.push_back(AZStd::move(link));
    }

    void PrefabProcessorContext::ResolveSpawnableEntityAliases(
        AZStd::string_view prefabName, AzFramework::Spawnable& spawnable, const AzToolsFramework::Prefab::Instance& instance)
    {
        using namespace AzToolsFramework::Prefab;

        for (EntityAliasStore& entityAlias : m_entityAliases)
        {
            auto sourcePrefab = AZStd::get_if<EntityAliasPrefabLink>(&entityAlias.m_source);
            if (sourcePrefab != nullptr && sourcePrefab->m_prefabName == prefabName)
            {
                AZ::EntityId id = instance.GetEntityIdFromAliasPath(sourcePrefab->m_alias);
                AZ_Assert(
                    id.IsValid(),
                    "Entity '%s' was not found in Prefab Instance created from '%s' even though it was previously found.",
                    sourcePrefab->m_alias.c_str(), sourcePrefab->m_prefabName.c_str());
                entityAlias.m_source.emplace<EntityAliasSpawnableLink>(spawnable, id);
            }

            auto targetPrefab = AZStd::get_if<EntityAliasPrefabLink>(&entityAlias.m_target);
            if (targetPrefab != nullptr && targetPrefab->m_prefabName == prefabName)
            {
                AZ::EntityId id = instance.GetEntityIdFromAliasPath(targetPrefab->m_alias);
                AZ_Assert(
                    id.IsValid(), "Entity '%s' was not found in Prefab Instance created from '%s' even though it was previously found.",
                    targetPrefab->m_alias.c_str(), targetPrefab->m_prefabName.c_str());
                entityAlias.m_target.emplace<EntityAliasSpawnableLink>(spawnable, id);
            }
        }
    }

    PrefabProcessorContext::ProcessedObjectStoreContainer& PrefabProcessorContext::GetProcessedObjects()
    {
        return m_products;
    }

    const PrefabProcessorContext::ProcessedObjectStoreContainer& PrefabProcessorContext::GetProcessedObjects() const
    {
        return m_products;
    }

    PrefabProcessorContext::ProductAssetDependencyContainer& PrefabProcessorContext::GetRegisteredProductAssetDependencies()
    {
        return m_registeredProductAssetDependencies;
    }

    const PrefabProcessorContext::ProductAssetDependencyContainer& PrefabProcessorContext::GetRegisteredProductAssetDependencies() const
    {
        return m_registeredProductAssetDependencies;
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

    void PrefabProcessorContext::ResolveLinks()
    {
        // Store the aliases visitor here when first encountered to avoid the visitor sorting the aliases for every addition.
        // Once this map goes out of scope the visitors will be destroyed and in turn sort their aliases.
        AZStd::unordered_map<AZ::Data::AssetId, AzFramework::Spawnable::EntityAliasVisitor> aliasVisitors;

        for (EntityAliasStore& alias : m_entityAliases)
        {
            auto source = AZStd::get_if<EntityAliasSpawnableLink>(&alias.m_source);
            AZ_Assert(source, "Entity alias found that has a source that's not yet resolved to a spawnable");
            auto target = AZStd::get_if<EntityAliasSpawnableLink>(&alias.m_target);
            AZ_Assert(target, "Entity alias found that has a target that's not yet resolved to a spawnable");

            uint32_t sourceIndex = SpawnableUtils::FindEntityIndex(source->m_index, source->m_spawnable);
            AZ_Assert(
                sourceIndex != SpawnableUtils::InvalidEntityIndex, "Entity %zu not found in source spawnable while resolving to index.",
                aznumeric_cast<AZ::u64>(source->m_index));
            uint32_t targetIndex = SpawnableUtils::FindEntityIndex(target->m_index, target->m_spawnable);
            AZ_Assert(
                targetIndex != SpawnableUtils::InvalidEntityIndex, "Entity %zu not found in target spawnable while resolving to index.",
                aznumeric_cast<AZ::u64>(target->m_index));

            AZ::Data::AssetLoadBehavior loadBehavior = ToAssetLoadBehavior(alias.m_loadBehavior);

            auto it = aliasVisitors.find(source->m_spawnable.GetId());
            if (it == aliasVisitors.end())
            {
                AzFramework::Spawnable::EntityAliasVisitor visitor = source->m_spawnable.TryGetAliases();
                AZ_Assert(visitor.IsValid(), "Unable to obtain lock for a newly create spawnable.");
                it = aliasVisitors.emplace(source->m_spawnable.GetId(), AZStd::move(visitor)).first;
            }
            it->second.AddAlias(
                AZ::Data::Asset<AzFramework::Spawnable>(&target->m_spawnable, loadBehavior), alias.m_tag, sourceIndex, targetIndex,
                alias.m_aliasType, alias.m_loadBehavior == EntityAliasSpawnableLoadBehavior::QueueLoad);

            // Register the dependency between the two spawnables.
            RegisterProductAssetDependency(source->m_spawnable.GetId(), target->m_spawnable.GetId(), loadBehavior);
        }
    }

    bool PrefabProcessorContext::HasCompletedSuccessfully() const
    {
        return m_completedSuccessfully;
    }

    void PrefabProcessorContext::ErrorEncountered()
    {
        m_completedSuccessfully = false;
    }

    AZ::Data::AssetLoadBehavior PrefabProcessorContext::ToAssetLoadBehavior(EntityAliasSpawnableLoadBehavior loadBehavior) const
    {
        return loadBehavior == EntityAliasSpawnableLoadBehavior::DependentLoad ? AZ::Data::AssetLoadBehavior::PreLoad
                                                                               : AZ::Data::AssetLoadBehavior::NoLoad;
    }

    const AzToolsFramework::Prefab::InstanceAlias& PrefabProcessorContext::FindOrCreateCachedInstanceAliasForPrefab(const AZStd::string& prefabName)
    {
        auto it = m_cachedPrefabInstanceAliases.find(prefabName);
        if (it != m_cachedPrefabInstanceAliases.end())
        {
            return it->second;
        }

        m_cachedPrefabInstanceAliases.insert({{prefabName, AzToolsFramework::Prefab::Instance::GenerateEntityAlias()}});
        return FindOrCreateCachedInstanceAliasForPrefab(prefabName);
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
