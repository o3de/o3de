/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
namespace AzToolsFramework
{
    class BaseSliceCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseSliceCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseSliceCommand, "{87CB1C80-7D75-423B-964A-CF1964A9AB26}");

        BaseSliceCommand(const AZStd::string& friendlyName);

        ~BaseSliceCommand() {};

        void Undo() override = 0;
        void Redo() override = 0;

        bool Changed() const override { return m_changed; }

    protected:

        bool CaptureRestoreInfoForUndo(const AZ::EntityId& entityId);

        void RestoreEntities(AZ::SliceComponent::EntityRestoreInfoList& entitiesToRestore,
            bool clearRestoreList = false,
            SliceEntityRestoreType restoreType = SliceEntityRestoreType::Detached);

        AZ::SliceComponent::EntityRestoreInfoList m_entityRedoRestoreInfoArray;
        AZ::SliceComponent::EntityRestoreInfoList m_entityUndoRestoreInfoArray;
        bool                                      m_changed;
    };
}
