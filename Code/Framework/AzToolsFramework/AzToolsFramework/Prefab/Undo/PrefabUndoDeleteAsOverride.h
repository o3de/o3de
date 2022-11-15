/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Undo/PrefabUndoUpdateLink.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        //! Undo class for handling deletion of entities and prefab instances to an instance as override of focused instance.
        class PrefabUndoDeleteAsOverride
            : public PrefabUndoUpdateLink
        {
        public:
            AZ_RTTI(PrefabUndoDeleteAsOverride, "{399C7D62-D748-4697-AB44-6BB478E3E128}", PrefabUndoUpdateLink);
            AZ_CLASS_ALLOCATOR(PrefabUndoDeleteAsOverride, AZ::SystemAllocator, 0);

            explicit PrefabUndoDeleteAsOverride(const AZStd::string& undoOperationName);

            //! The function helps generate undo/redo patches for deleting entities and prefab instances.
            void Capture(
                const AZStd::vector<AZStd::string>& entityAliasPathList,
                const AZStd::vector<AZStd::string>& instanceAliasPathList,
                const AZStd::vector<const AZ::Entity*> parentEntityList,
                const Instance& owningInstance,
                Instance& focusedInstance);
        };
    } // namespace Prefab
} // namespace AzToolsFramework
