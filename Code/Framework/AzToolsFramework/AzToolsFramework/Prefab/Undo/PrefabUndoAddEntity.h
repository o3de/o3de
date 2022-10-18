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
        class PrefabSystemComponentInterface;

        //! Undo class for handling addition of an entity to a prefab template.
        class PrefabUndoAddEntity
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoAddEntity, "{67EC7123-7F42-4BDD-9543-43349E2EA605}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoAddEntity, AZ::SystemAllocator, 0);

            explicit PrefabUndoAddEntity(const AZStd::string& undoOperationName);

            void Capture(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                TemplateId focusedTemplateId,
                const AZStd::string& focusedToOwningInstancePath,
                PrefabDomReference cachedInstanceDom);

            void Undo() override;
            void Redo() override;

        private:
            void GenerateUpdateParentEntityUndoPatches(
                const PrefabDom& parentEntityDomAfterAddingEntity,
                const AZStd::string& parentEntityAliasPath);

            void GenerateAddNewEntityUndoPatches(
                const PrefabDom& newEntityDom,
                const AZStd::string& newEntityAliasPath);

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
