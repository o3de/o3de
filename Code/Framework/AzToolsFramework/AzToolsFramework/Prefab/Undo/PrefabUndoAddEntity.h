/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabDomUtils.h>
#include <Prefab/Undo/PrefabUndoBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabSystemComponentInterface;

        //! Undo class for handling addition of an entity to a prefab template.
        class PrefabUndoAddEntity
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoAddEntity, "{67EC7123-7F42-4BDD-9543-43349E2EA605}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoAddEntity, AZ::SystemAllocator, 0);

            explicit PrefabUndoAddEntity(const AZStd::string& undoOperationName);

            // The function help generate undo/redo patches for adding the a new entity under a target parent entity,
            // where both entities are under the current focused prefab instance.
            void Capture(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& focusedInstance);

            void Undo() override;
            void Redo() override;

        private:
            void UpdateLink(PrefabDom& linkDom);

            void UpdateCachedOwningInstanceDOM(
                PrefabDomReference cachedOwningInstanceDom,
                const PrefabDom& entityDom,
                const AZStd::string& entityAliasPath);

            void GeneratePatchesForUpdateParentEntity(
                PrefabDom& parentEntityDomAfterAddingEntity,
                const AZStd::string& parentEntityAliasPathForPatches,
                const AZStd::string& parentEntityAliasPathInFocusedTemplate,
                bool updateUndoPatchNeeded);

            void GeneratePatchesForAddNewEntity(
                PrefabDom& newEntityDom,
                const AZStd::string& newEntityAliasPathForPatches,
                bool updateUndoPatchNeeded);

            void GeneratePatchesForLinkUpdate();

            LinkReference m_link = AZStd::nullopt;
            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
