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
     * EditorOnlyEntity handler for world entities.
     * - Fixes up transform relationships so entities removed mid-hierarchy still result in valid runtime transform relationships
     *   and correct relative transforms.
     * - Validates that no editor entities are referenced by non-editor entities.
     */
    class WorldEditorOnlyEntityHandler
        : public EditorOnlyEntityHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldEditorOnlyEntityHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::WorldEditorOnlyEntityHandler, "{55587AE2-B583-48E4-9634-6BFACF6CBF04}", EditorOnlyEntityHandler);

        bool IsEntityUniquelyForThisHandler(AZ::Entity* entity) const override;

        Result HandleEditorOnlyEntities(
            const AzToolsFramework::EntityList& entities,
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds,
            AZ::SerializeContext& serializeContext) override;

        // Adjust transform relationships to maintain integrity of the transform hierarchy at runtime, even if editor-only 
        // entities were positioned within the transform hierarchy.
        static void FixTransformRelationships(
            const AzToolsFramework::EntityList& entities,
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds);
    };

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
