/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/WorldEditorOnlyEntityHandler.h>

#include <AzCore/Component/TransformBus.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    bool WorldEditorOnlyEntityHandler::IsEntityUniquelyForThisHandler(AZ::Entity* entity) const
    {
        return AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(entity) != nullptr;
    }

    EditorOnlyEntityHandler::Result WorldEditorOnlyEntityHandler::HandleEditorOnlyEntities(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, AZ::SerializeContext& serializeContext)
    {
        FixTransformRelationships(entities, editorOnlyEntityIds);

        return ValidateReferences(entities, editorOnlyEntityIds, serializeContext);
    }

    void WorldEditorOnlyEntityHandler::FixTransformRelationships(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds)
    {
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>> parentToChildren;

        // Build a map of entity Ids to their parent Ids, for faster lookup during processing.
        for (AZ::Entity* entity : entities)
        {
            AZ::TransformInterface* transformComponent = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(entity);
            if (transformComponent)
            {
                const AZ::EntityId parentId = transformComponent->GetParentId();
                if (parentId.IsValid())
                {
                    parentToChildren[parentId].push_back(entity);
                }
            }
        }

        // Identify any editor-only entities. If we encounter one, adjust transform relationships
        // for all of its children to ensure relative transforms are maintained and respected at
        // runtime.
        // This works regardless of entity ordering in the Prefab because we add reassigned children to 
        // parentToChildren cache during the operation.
        for (AZ::Entity* entity : entities)
        {
            if (editorOnlyEntityIds.end() == editorOnlyEntityIds.find(entity->GetId()))
            {
                continue; // This is not an editor-only entity.
            }

            AZ::TransformInterface* transformComponent = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(entity);
            if (transformComponent)
            {
                const AZ::Transform& parentLocalTm = transformComponent->GetLocalTM();

                // Identify all transform children and adjust them to be children of the removed entity's parent.
                for (AZ::Entity* childEntity : parentToChildren[entity->GetId()])
                {
                    AZ::TransformInterface* childTransformComponent = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(childEntity);

                    if (childTransformComponent && childTransformComponent->GetParentId() == entity->GetId())
                    {
                        const AZ::Transform localTm = childTransformComponent->GetLocalTM();
                        childTransformComponent->SetParent(transformComponent->GetParentId());
                        childTransformComponent->SetLocalTM(parentLocalTm * localTm);

                        parentToChildren[transformComponent->GetParentId()].push_back(childEntity);
                    }
                }
            }
        }
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
