/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/EditorOnlyEntityHandler.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    void EditorOnlyEntityHandler::AddEditorOnlyEntity(
        AZ::Entity* editorOnlyEntity,
        AZStd::unordered_set<AZ::EntityId>& editorOnlyEntities)
    {
        editorOnlyEntities.insert(editorOnlyEntity->GetId());
    }

    EditorOnlyEntityHandler::Result EditorOnlyEntityHandler::HandleEditorOnlyEntities(
        const EntityList& /*entities*/,
        const EntityIdSet& /*editorOnlyEntityIds*/,
        AZ::SerializeContext& /*serializeContext*/)
    {
        return AZ::Success();
    }

    EditorOnlyEntityHandler::Result EditorOnlyEntityHandler::ValidateReferences(
        const EntityList& entities,
        const EntityIdSet& editorOnlyEntityIds,
        AZ::SerializeContext& serializeContext)
    {
        EditorOnlyEntityHandler::Result result = AZ::Success();

        // Inspect all runtime entities via the serialize context and identify any references to editor-only entity Ids.
        for (AZ::Entity* runtimeEntity : entities)
        {
            if (editorOnlyEntityIds.end() != editorOnlyEntityIds.find(runtimeEntity->GetId()))
            {
                continue; // This is not a runtime entity, so no need to validate its references as it's going away.
            }

            AZ::EntityUtils::EnumerateEntityIds<AZ::Entity>(
                runtimeEntity,
                [&editorOnlyEntityIds, &result, runtimeEntity](const AZ::EntityId& id, bool /*isEntityId*/, const AZ::SerializeContext::ClassElement* /*elementData*/)
                {
                    if (editorOnlyEntityIds.end() != editorOnlyEntityIds.find(id))
                    {
                        result = AZ::Failure(
                            AZStd::string::format(
                                "A runtime entity (%s) contains references to an entity marked as editor-only.",
                                runtimeEntity->GetName().c_str()
                            )
                        );

                        return false;
                    }

                    return true;
                },
                &serializeContext
            );

            if (!result)
            {
                break;
            }
        }

        return result;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
