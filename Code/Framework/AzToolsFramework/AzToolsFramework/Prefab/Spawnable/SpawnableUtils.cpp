/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace AzToolsFramework::Prefab::SpawnableUtils
{
    bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom)
    {
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> referencedAssets;
        return CreateSpawnable(spawnable, prefabDom, referencedAssets);
    }

    bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom, AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets)
    {
        Instance instance;
        if (Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(instance, prefabDom, referencedAssets,
            Prefab::PrefabDomUtils::LoadInstanceFlags::AssignRandomEntityId)) // Always assign random entity ids because the spawnable is
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
