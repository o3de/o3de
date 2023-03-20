/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>


namespace AzToolsFramework::Prefab::SpawnableUtils
{
    namespace Internal
    {
        AZ::SerializeContext* GetSerializeContext()
        {
            AZ::SerializeContext* result = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(result, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(result, "SpawnbleUtils was unable to locate the Serialize Context.");
            return result;
        }

        AZ::Entity* FindEntity(AZ::EntityId entityId, AzToolsFramework::Prefab::Instance& source)
        {
            AZ::Entity* result = nullptr;
            source.GetEntities(
                [&result, entityId](AZStd::unique_ptr<AZ::Entity>& entity)
                {
                    if (entity->GetId() != entityId)
                    {
                        return true;
                    }
                    else
                    {
                        result = entity.get();
                        return false;
                    }
                });
            return result;
        }

        const AZ::Entity* FindEntity(AZ::EntityId entityId, const AzToolsFramework::Prefab::Instance& source)
        {
            const AZ::Entity* result = nullptr;
            source.GetConstEntities(
                [&result, entityId](const AZ::Entity& entity)
                {
                    if (entity.GetId() != entityId)
                    {
                        return true;
                    }
                    else
                    {
                        result = &entity;
                        return false;
                    }
                });
            return result;
        }

        AZ::Entity* FindEntity(AZ::EntityId entityId, AzFramework::Spawnable& source)
        {
            uint32_t index = AzToolsFramework::Prefab::SpawnableUtils::FindEntityIndex(entityId, source);
            return index != InvalidEntityIndex ? source.GetEntities()[index].get() : nullptr;
        }

        AZStd::unique_ptr<AZ::Entity> CloneEntity(AZ::EntityId entityId, AzToolsFramework::Prefab::Instance& source)
        {
            AZ::Entity* target = Internal::FindEntity(entityId, source);
            AZ_Assert(
                target, "SpawnbleUtils were unable to locate entity with id %zu in Instance or Spawnable for cloning.",
                aznumeric_cast<AZ::u64>(entityId));
            auto clone = AZStd::make_unique<AZ::Entity>();

            static AZ::SerializeContext* sc = GetSerializeContext();
            sc->CloneObjectInplace(*clone, target);
            clone->SetId(AZ::Entity::MakeId());

            return clone;        
        }

        AZStd::unique_ptr<AZ::Entity> ReplaceEntityWithPlaceholder(
            AZ::EntityId entityId,
            [[maybe_unused]] AZStd::string_view sourcePrefabName,
            AzToolsFramework::Prefab::Instance& source)
        {
            auto&& [instance, alias] = source.FindInstanceAndAlias(entityId);
            AZ_Assert(
                instance, "SpawnbleUtils were unable to locate entity alias with id %zu in Instance '%.*s' for replacing.",
                aznumeric_cast<AZ::u64>(entityId), AZ_STRING_ARG(sourcePrefabName));

            EntityOptionalReference entityData = instance->GetEntity(alias);
            AZ_Assert(
                entityData.has_value(), "SpawnbleUtils were unable to locate entity '%.*s' in Instance '%.*s' for replacing.",
                AZ_STRING_ARG(alias), AZ_STRING_ARG(sourcePrefabName));
            // A new entity id can be used for the placeholder as `ReplaceEntity` will swap the entity ids.
            auto placeholder = AZStd::make_unique<AZ::Entity>(AZ::Entity::MakeId(), entityData->get().GetName());
            // Keep a transform component on the placeholder to maintain parent/child relationship.
            // This is used during prefab processing to sort the corresponding spawnable's entities by hierarchy
            auto transformComponent = aznew AzFramework::TransformComponent();
            placeholder->AddComponent(transformComponent);
            auto entityTransformComponent = entityData->get().FindComponent<AzFramework::TransformComponent>();
            transformComponent->SetParentRelative(entityTransformComponent->GetParentId());
            return instance->ReplaceEntity(AZStd::move(placeholder), alias);
        }

        AZStd::pair<AZStd::unique_ptr<AZ::Entity>, AzFramework::Spawnable::EntityAliasType> ApplyAlias(
            AZStd::string_view sourcePrefabName,
            AzToolsFramework::Prefab::Instance& source,
            AZ::EntityId entityId,
            AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasType aliasType)
        {
            namespace PCU = AzToolsFramework::Prefab::PrefabConversionUtils;
            using ResultPair = AZStd::pair<AZStd::unique_ptr<AZ::Entity>, AzFramework::Spawnable::EntityAliasType>;

            switch (aliasType)
            {
            case PCU::EntityAliasType::Disable:
                // No need to do anything as the alias is disabled.
                return ResultPair(nullptr, AzFramework::Spawnable::EntityAliasType::Disable);
            case PCU::EntityAliasType::OptionalReplace:
                return ResultPair(CloneEntity(entityId, source), AzFramework::Spawnable::EntityAliasType::Replace);
            case PCU::EntityAliasType::Replace:
                return ResultPair(
                    ReplaceEntityWithPlaceholder(entityId, sourcePrefabName, source),
                    AzFramework::Spawnable::EntityAliasType::Replace);
            case PCU::EntityAliasType::Additional:
                return ResultPair(AZStd::make_unique<AZ::Entity>(), AzFramework::Spawnable::EntityAliasType::Additional);
            case PCU::EntityAliasType::Merge:
                return ResultPair(AZStd::make_unique<AZ::Entity>(), AzFramework::Spawnable::EntityAliasType::Merge);
            default:
                AZ_Assert(
                    false, "Invalid PrefabProcessorContext::EntityAliasType type (%i) provided.", aznumeric_cast<uint64_t>(aliasType));
                return ResultPair(nullptr, AzFramework::Spawnable::EntityAliasType::Disable);
            }
        }
    }

    bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom)
    {
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> referencedAssets;
        return CreateSpawnable(spawnable, prefabDom, referencedAssets);
    }

    bool CreateSpawnable(
        AzFramework::Spawnable& spawnable,
        const PrefabDom& prefabDom,
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets)
    {
        Instance instance;
        if (Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(
                instance, prefabDom, referencedAssets,
                Prefab::PrefabDomUtils::LoadFlags::AssignRandomEntityId)) // Always assign random entity ids because the spawnable is
                                                                          // going to be used to create clones of the entities.
        {
            AzFramework::Spawnable::EntityList& entities = spawnable.GetEntities();
            instance.DetachAllEntitiesInHierarchy(
                [&entities](AZStd::unique_ptr<AZ::Entity> entity)
                {
                    entities.emplace_back(AZStd::move(entity));
                });
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ::Entity* CreateEntityAlias(
        AZStd::string sourcePrefabName,
        AzToolsFramework::Prefab::Instance& source,
        AZStd::string targetPrefabName,
        AzToolsFramework::Prefab::Instance& target,
        AzToolsFramework::Prefab::Instance& targetNestedInstance,
        AZ::EntityId entityId,
        AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasType aliasType,
        AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasSpawnableLoadBehavior loadBehavior,
        uint32_t tag,
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context)
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        AliasPath alias = source.GetAliasPathRelativeToInstance(entityId);
        if (!alias.empty())
        {
            auto&& [replacement, storedAliasType] =
                Internal::ApplyAlias(sourcePrefabName, source, entityId, aliasType);
            if (replacement)
            {
                AZ::Entity* result = replacement.get();
                targetNestedInstance.AddEntity(AZStd::move(replacement), alias.Filename().Native());

                EntityAliasStore store;
                store.m_aliasType = storedAliasType;
                store.m_source.emplace<EntityAliasPrefabLink>(AZStd::move(sourcePrefabName), AZStd::move(alias));
                store.m_target.emplace<EntityAliasPrefabLink>(
                    AZStd::move(targetPrefabName), target.GetAliasPathRelativeToInstance(result->GetId()));
                store.m_loadBehavior = loadBehavior;
                store.m_tag = tag;
                context.RegisterSpawnableEntityAlias(AZStd::move(store));

                return result;
            }
            else
            {
                AZ_Assert(false, "A replacement for entity with id %zu could not be created.", static_cast<AZ::u64>(entityId));
                return nullptr;
            }
        }
        else
        {
            AZ_Assert(false, "Entity with id %zu was not found in the source prefab.", static_cast<AZ::u64>(entityId));
            return nullptr;
        }
    }

    uint32_t FindEntityIndex(AZ::EntityId entity, const AzFramework::Spawnable& spawnable)
    {
        auto begin = spawnable.GetEntities().begin();
        auto end = spawnable.GetEntities().end();
        for(auto it = begin; it != end; ++it)
        {
            if ((*it)->GetId() == entity)
            {
                return aznumeric_caster(AZStd::distance(begin, it));
            }
        }
        return InvalidEntityIndex;
    }

    template<typename EntityPtr>
    void OrganizeEntitiesForSorting(
        AZStd::vector<EntityPtr>& entities,
        AZStd::unordered_set<AZ::EntityId>& existingEntityIds,
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<EntityPtr>>& parentIdToChildren,
        AZStd::vector<AZ::EntityId>& candidateIds,
        size_t& removedEntitiesCount)
    {
        existingEntityIds.clear();
        parentIdToChildren.clear();
        candidateIds.clear();
        removedEntitiesCount = 0;

        for (auto& entity : entities)
        {
            if (!entity)
            {
                ++removedEntitiesCount;
                continue;
            }

            AZ::EntityId entityId = entity->GetId();
            if (!entityId.IsValid())
            {
                AZ_Warning("Entity", false, "Hierarchy sort found entity '%s' with invalid ID", entity->GetName().c_str());

                ++removedEntitiesCount;
                continue;
            }

            if (!existingEntityIds.insert(entityId).second)
            {
                AZ_Warning("Entity", false, "Hierarchy sort found multiple entities using same ID as entity '%s' %s",
                    entity->GetName().c_str(),
                    entityId.ToString().c_str());

                ++removedEntitiesCount;
                continue;
            }

            // search for any component that implements the TransformInterface.
            // don't use EBus because we support sorting entities that haven't been initialized or activated.
            // entities with no transform component will be treated like entities with no parent.
            AZ::EntityId parentId;
            if (AZ::TransformInterface* transformInterface =
                AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(&(*entity)))
            {
                parentId = transformInterface->GetParentId();
                if (parentId == entityId)
                {
                    AZ_Warning("Entity", false, "Hierarchy sort found entity parented to itself '%s' %s",
                        entity->GetName().c_str(),
                        entityId.ToString().c_str());

                    parentId.SetInvalid();
                }
            }

            auto& children = parentIdToChildren[parentId];
            children.emplace_back(AZStd::move(entity));
        }

        // clear 'entities', we'll refill it in sorted order.
        entities.clear();

        // the first candidates should be the parents of the roots.
        for (auto& parentChildrenPair : parentIdToChildren)
        {
            const AZ::EntityId& parentId = parentChildrenPair.first;

            // we found a root if parent ID doesn't correspond to any entity in the list
            if (existingEntityIds.find(parentId) == existingEntityIds.end())
            {
                candidateIds.push_back(parentId);
            }
        }
        
    }

    template<typename EntityPtr>
    void TraceParentingLoop(
        const AZ::EntityId& parentFromLoopId,
        const AZStd::unordered_map<AZ::EntityId, AZStd::vector<EntityPtr>>& parentIdToChildren)
    {

        // Find name to use in warning message
        AZStd::string_view parentFromLoopName;
        for (const auto& parentIdChildrenPair : parentIdToChildren)
        {
            for (const auto& entity : parentIdChildrenPair.second)
            {
                if (entity->GetId() == parentFromLoopId)
                {
                    parentFromLoopName = entity->GetName();
                    break;
                }
                if (!parentFromLoopName.empty())
                {
                    break;
                }
            }
        }

        AZ_Warning("Entity", false, "Hierarchy sort found parenting loop involving entity '%.*s' %s",
            AZ_STRING_ARG(parentFromLoopName),
            parentFromLoopId.ToString().c_str());
    }


    void SortEntitiesByTransformHierarchy(AzFramework::Spawnable& spawnable)
    {
        SortEntitiesByTransformHierarchy(spawnable.GetEntities());
    }

    template<typename EntityPtr>
    void SortEntitiesByTransformHierarchy(AZStd::vector<EntityPtr>& entities)
    {
        const size_t originalEntityCount = entities.size();

        // IDs of those present in 'entities'. Does not include parent ID if parent not found in 'entities'
        AZStd::unordered_set<AZ::EntityId> existingEntityIds;

        // map children by their parent ID (even if parent not found in 'entities')
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<EntityPtr>> parentIdToChildren;

        // use 'candidateIds' to track the parent IDs we're going to process next.
        AZStd::vector<AZ::EntityId> candidateIds;
        candidateIds.reserve(originalEntityCount + 1);

        size_t removedCount = 0;
        OrganizeEntitiesForSorting(entities, existingEntityIds, parentIdToChildren, candidateIds, removedCount);

        // process candidates until everything is sorted:
        // - add candidate's children to the final sorted order
        // - add candidate's children to list of candidates, so we can process *their* children in a future loop
        // - erase parent/children entry from parentToChildrenIds
        // - continue until nothing is left in parentToChildrenIds
        for (size_t candidateIndex = 0; !parentIdToChildren.empty(); ++candidateIndex)
        {
            // if there are no more candidates, but there are still unsorted children, then we have an infinite loop.
            // pick an arbitrary parent from the loop to be the next candidate.
            if (candidateIndex == candidateIds.size())
            {
                const AZ::EntityId& parentFromLoopId = parentIdToChildren.begin()->first;

#ifdef AZ_ENABLE_TRACING
                TraceParentingLoop(parentFromLoopId, parentIdToChildren);
#endif // AZ_ENABLE_TRACING

                candidateIds.push_back(parentFromLoopId);
            }

            const AZ::EntityId& parentId = candidateIds[candidateIndex];

            auto foundChildren = parentIdToChildren.find(parentId);
            if (foundChildren != parentIdToChildren.end())
            {
                for (auto& child : foundChildren->second)
                {
                    candidateIds.push_back(child->GetId());
                    entities.emplace_back(AZStd::move(child));
                }

                parentIdToChildren.erase(foundChildren);
            }
        }


        AZ_Assert(entities.size() + removedCount == originalEntityCount,
            "Wrong number of entities after sort. Original entity count = %zu, Sorted entity count = %zu, Removed entity count = %zu",
            originalEntityCount,
            entities.size(),
            removedCount
        );

    }

    // Explicit specializations of SortEntitiesByTransformHierarchy (have to be in cpp due to clang errors)
    template void SortEntitiesByTransformHierarchy(AZStd::vector<AZ::Entity*>& entities);
    template void SortEntitiesByTransformHierarchy(AZStd::vector<AZStd::unique_ptr<AZ::Entity>>& entities);
} // namespace AzToolsFramework::Prefab::SpawnableUtils

