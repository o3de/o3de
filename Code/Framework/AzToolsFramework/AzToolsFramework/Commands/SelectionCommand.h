/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZTOOLSFRAMEWORK_SELECTIONCOMMAND_H
#define AZTOOLSFRAMEWORK_SELECTIONCOMMAND_H

#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework
{
    /**
     * Stores an entity selection set for undo/redo.
     */
    class SelectionCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(SelectionCommand, AZ::SystemAllocator);
        AZ_RTTI(SelectionCommand, "{07A0CF6A-79FA-4EA3-B056-1C0DA6F36699}", UndoSystem::URSequencePoint);

        SelectionCommand(const AZStd::vector<AZ::EntityId>& proposedSelection, const AZStd::string& friendlyName);

        virtual void UpdateSelection(const AZStd::vector<AZ::EntityId>& proposedSelection);

        virtual void Post();

        void Undo() override;
        void Redo() override;

        bool Changed() const override { return m_previousSelectionList != m_proposedSelectionList; }

        const AZStd::vector<AZ::EntityId>& GetInitialSelectionList() const;

    protected:
        EntityIdList m_previousSelectionList;
        EntityIdList m_proposedSelectionList;
    };
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_SELECTIONCOMMAND_H
