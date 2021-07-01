/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/UiEditorOnlyEntityHandler.h>

#include <AzFramework/InGameUI/UiFrameworkBus.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    bool UiEditorOnlyEntityHandler::IsEntityUniquelyForThisHandler(AZ::Entity* entity) const
    {
        // Assume that an entity is a UI element if it has a UI element component.
        bool uniqueForThisHandler = false;
        UiFrameworkBus::BroadcastResult(uniqueForThisHandler, &UiFrameworkInterface::HasUiElementComponent, entity);

        return uniqueForThisHandler;
    }

    void UiEditorOnlyEntityHandler::AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities)
    {
        UiFrameworkBus::Broadcast(&UiFrameworkInterface::AddEditorOnlyEntity, editorOnlyEntity, editorOnlyEntities);
    }

    EditorOnlyEntityHandler::Result UiEditorOnlyEntityHandler::HandleEditorOnlyEntities(
        const AzToolsFramework::EntityList& exportEntities,
        const AzToolsFramework::EntityIdSet& editorOnlyEntityIds,
        AZ::SerializeContext& serializeContext)
    {
        UiFrameworkBus::Broadcast(&UiFrameworkInterface::HandleEditorOnlyEntities, exportEntities, editorOnlyEntityIds);

        // Perform a final check to verify that all editor-only entities have been removed
        auto result = ValidateReferences(exportEntities, editorOnlyEntityIds, serializeContext);
        return result;
    }

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
