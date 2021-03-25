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

#pragma once

#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/EditorOnlyEntityHandler.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    /**
    * EditorOnlyEntity handler for UI entities.
    * - Removes editor-only entities and their descedent hierarchy entirely.
    * -- This differs from the world-entity handler where editor-only entities
    *    are removed "in-place".
    * - Validates that no editor entities are referenced by non-editor entities.
    */
    class UiEditorOnlyEntityHandler
        : public EditorOnlyEntityHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(UiEditorOnlyEntityHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::UiEditorOnlyEntityHandler, "{949CF813-4A8E-4D55-B323-0ED2A967CDCC}", EditorOnlyEntityHandler);

        bool IsEntityUniquelyForThisHandler(AZ::Entity* entity) const override;

        void AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities) override;

        Result HandleEditorOnlyEntities(
            const AzToolsFramework::EntityList& entities,
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds,
            AZ::SerializeContext& serializeContext) override;
    };

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
