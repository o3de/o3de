/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    /**
     * Callback handler interface for processing prefab prior to stripping of editor-only entities.
     */
    class EditorOnlyEntityHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorOnlyEntityHandler, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::EditorOnlyEntityHandler, "{C420F65D-18AE-4CAF-BB18-70FA4FE73243}");

        virtual ~EditorOnlyEntityHandler() = default;

        virtual bool IsEntityUniquelyForThisHandler(AZ::Entity* entity) const = 0;

        /**
         * Adds the given entity ID to the set of editor only entities.
         *
         * Handlers can customize this behavior, such as additionally adding child entities
         * when a parent is marked as editor-only.
         */
        virtual void AddEditorOnlyEntity(
            AZ::Entity* editorOnlyEntity,
            AZStd::unordered_set<AZ::EntityId>& editorOnlyEntities);

        using Result = AZ::Outcome<void, AZStd::string>;

        /**
         * This handler is responsible for making any necessary modifications to other entities in the Prefab prior to the removal
         * of all editor-only entities.
         * After this callback returns, editor-only entities will be removed from the Prefab.
         * See \ref WorldEditorOnlyEntityHandler below for an example of processing and validation that occurs for standard world entities.
         * @param entities a list of all entities in the Prefab, including those marked as editor-only.
         * @param editorOnlyEntityIds a precomputed set containing Ids for all entities within the 'entities' list that were marked as editor-only.
         * @param serializeContext useful to inspect entity data for validation purposes.
         */
        virtual Result HandleEditorOnlyEntities(
            const EntityList& /*entities*/,
            const EntityIdSet& /*editorOnlyEntityIds*/,
            AZ::SerializeContext& /*serializeContext*/);

        // Verify that none of the runtime entities reference editor-only entities. Fail w/ details if so.
        static Result ValidateReferences(
            const EntityList& entities,
            const EntityIdSet& editorOnlyEntityIds,
            AZ::SerializeContext& serializeContext);
    };

    using EditorOnlyEntityHandlers = AZStd::vector<EditorOnlyEntityHandler*>;

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
