/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework::Prefab
{
    //! Undo node for prefab focus change operations.
    class AZTF_API PrefabFocusUndo
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusUndo, AZ::SystemAllocator)
        using ApplyFocus = AZStd::function<void(const RootAliasPath&)>;
        PrefabFocusUndo(RootAliasPath before, RootAliasPath after, ApplyFocus apply);

        bool Changed() const override;

        void Undo() override;
        void Redo() override;

    private:
        RootAliasPath m_before;
        RootAliasPath m_after;
        ApplyFocus m_apply;
    };
} // namespace AzToolsFramework::Prefab
