/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(UiEditorOnlyEntityHandler, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::UiEditorOnlyEntityHandler, "{949CF813-4A8E-4D55-B323-0ED2A967CDCC}", EditorOnlyEntityHandler);

        bool IsEntityUniquelyForThisHandler(AZ::Entity* entity) const override;

        void AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities) override;

        Result HandleEditorOnlyEntities(
            const AzToolsFramework::EntityList& entities,
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds,
            AZ::SerializeContext& serializeContext) override;
    };

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
