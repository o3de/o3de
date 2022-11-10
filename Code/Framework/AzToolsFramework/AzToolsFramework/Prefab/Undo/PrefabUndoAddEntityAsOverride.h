/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUpdateLink.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabSystemComponentInterface;

        //! Undo class for handling addition of an entity to an instance as override of focused instance.
        class PrefabUndoAddEntityAsOverride
            : public PrefabUndoUpdateLink
        {
        public:
            AZ_RTTI(PrefabUndoAddEntityAsOverride, "{45CD5DB2-7A78-45A0-AC85-D2F48F90CA1E}", PrefabUndoUpdateLink);
            AZ_CLASS_ALLOCATOR(PrefabUndoAddEntityAsOverride, AZ::SystemAllocator, 0);

            explicit PrefabUndoAddEntityAsOverride(const AZStd::string& undoOperationName);

            // The function help generate undo/redo patches for adding the a new entity under a target parent entity,
            // where both entities are under the given owning prefab instance, and the owning instance is a descendant
            // of the current focused instance.
            void Capture(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& owningInstance,
                Instance& focusedInstance);
        };
    }
}
