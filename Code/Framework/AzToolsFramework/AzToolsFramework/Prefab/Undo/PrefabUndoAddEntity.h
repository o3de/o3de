/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Undo/PrefabUndoBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        //! Undo class for handling addition of an entity to a prefab instance.
        class PrefabUndoAddEntity
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoAddEntity, "{67EC7123-7F42-4BDD-9543-43349E2EA605}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoAddEntity, AZ::SystemAllocator);

            explicit PrefabUndoAddEntity(const AZStd::string& undoOperationName);

            // The function help generate undo/redo patches for adding the a new entity under a target parent entity,
            // where both entities are under the current focused prefab instance.
            void Capture(
                const AZ::Entity& parentEntity,
                const AZ::Entity& newEntity,
                Instance& focusedInstance);
        };
    }
}
