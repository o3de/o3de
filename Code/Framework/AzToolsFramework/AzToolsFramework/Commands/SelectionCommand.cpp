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
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(m_previousSelectionList,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

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
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(undoStack,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetUndoStack);

        if (undoStack)
        {
            undoStack->Post(this);
        }
    }

    void SelectionCommand::Undo()
    {
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities, m_previousSelectionList);
    }

    void SelectionCommand::Redo()
    {
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities, m_proposedSelectionList);
    }
}
