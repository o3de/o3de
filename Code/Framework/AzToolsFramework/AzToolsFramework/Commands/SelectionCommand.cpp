/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SelectionCommand.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    SelectionCommand::SelectionCommand(const AZStd::vector<AZ::EntityId>& proposedSelection, const AZStd::string& friendlyName)
        : UndoSystem::URSequencePoint(friendlyName)
    {
        EBUS_EVENT_RESULT(m_previousSelectionList, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);

        m_proposedSelectionList = proposedSelection;
    }

    const AZStd::vector<AZ::EntityId>& SelectionCommand::GetInitialSelectionList() const
    {
        return m_previousSelectionList;
    }

    void SelectionCommand::UpdateSelection(const EntityIdList& proposedSelection)
    {
        if (m_proposedSelectionList != proposedSelection)
        {
            m_proposedSelectionList = proposedSelection;
        }
    }

    void SelectionCommand::Post()
    {
        UndoSystem::UndoStack* undoStack = nullptr;
        EBUS_EVENT_RESULT(undoStack, AzToolsFramework::ToolsApplicationRequests::Bus, GetUndoStack);

        if (undoStack)
        {
            undoStack->Post(this);
        }
    }

    void SelectionCommand::Undo()
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, m_previousSelectionList);
    }

    void SelectionCommand::Redo()
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, m_proposedSelectionList);
    }
}
