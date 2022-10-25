/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabDomUtils.h>
#include <Prefab/Undo/PrefabUndoUpdateLinkBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabSystemComponentInterface;

        //! Undo class for handling addition of an entity to an instance which is a descendant of focused instance.
        class PrefabUndoUnfocusedInstanceAddEntity
            : public PrefabUndoUpdateLinkBase
        {
        public:
            AZ_RTTI(PrefabUndoUnfocusedInstanceAddEntity, "{45CD5DB2-7A78-45A0-AC85-D2F48F90CA1E}", PrefabUndoUpdateLinkBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoUnfocusedInstanceAddEntity, AZ::SystemAllocator, 0);

            explicit PrefabUndoUnfocusedInstanceAddEntity(const AZStd::string& undoOperationName);

            // The function help generate undo/redo patches for adding the a new entity under a target parent entity,
            // where both entities are under the current focused prefab instance.
            void Capture(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& owningInstance,
                Instance& focusedInstance);

        private:
            void GeneratePatchesForUpdateParentEntity(
                PrefabDom& parentEntityDomAfterAddingEntity,
                const AZStd::string& parentEntityAliasPathForPatches,
                const AZStd::string& parentEntityAliasPathInFocusedTemplate);
        };
    }
}
