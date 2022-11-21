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
        //! Undo class for handling deletion of entities to the focused instance.
        class PrefabUndoDeleteEntity
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoDeleteEntity, "{63C3579D-9F8B-4B0F-AEE9-F8CF2E3E941A}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoDeleteEntity, AZ::SystemAllocator, 0);

            explicit PrefabUndoDeleteEntity(const AZStd::string& undoOperationName);

            //! The function helps generate undo/redo patches for deleting entities.
            void Capture(
                const AZStd::vector<AZStd::string>& entityAliasPathList,
                const AZStd::vector<const AZ::Entity*> parentEntityList,
                Instance& focusedInstance);
        };
    } // namespace Prefab
} // namespace AzToolsFramework
