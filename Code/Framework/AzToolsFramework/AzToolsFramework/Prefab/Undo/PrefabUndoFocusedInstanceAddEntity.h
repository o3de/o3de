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

        //! Undo class for handling addition of an entity to a focused prefab instance.
        class PrefabUndoFocusedInstanceAddEntity
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoFocusedInstanceAddEntity, "{67EC7123-7F42-4BDD-9543-43349E2EA605}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoFocusedInstanceAddEntity, AZ::SystemAllocator, 0);

            explicit PrefabUndoFocusedInstanceAddEntity(const AZStd::string& undoOperationName);

            // The function help generate undo/redo patches for adding the a new entity under a target parent entity,
            // where both entities are under the current focused prefab instance.
            void Capture(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& focusedInstance);

        private:
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
