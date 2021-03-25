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
