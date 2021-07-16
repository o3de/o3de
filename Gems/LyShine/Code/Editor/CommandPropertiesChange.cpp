/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

CommandPropertiesChange::CommandPropertiesChange(UndoStack* stack,
    HierarchyWidget* hierarchy,
    SerializeHelpers::SerializedEntryList& preValueChanges,
    const char* commandName)
    : QUndoCommand()
    , m_stack(stack)
    , m_isFirstExecution(true)
    , m_hasPreviouslyFailed(false)
    , m_hierarchy(hierarchy)
    , m_entryList(HierarchyClipboard::Serialize(m_hierarchy, m_hierarchy->selectedItems(), nullptr, preValueChanges, false))
{
    AZ_Assert(!m_entryList.empty(), "Entry list is empty");

    setText(commandName);
}

void CommandPropertiesChange::undo()
{
    UndoStackExecutionScope s(m_stack);

    Recreate(true);
}

void CommandPropertiesChange::redo()
{
    UndoStackExecutionScope s(m_stack);

    Recreate(false);
}

void CommandPropertiesChange::Recreate(bool isUndo)
{
    if (m_hasPreviouslyFailed)
    {
        // Disable this command.
        // Nothing else to do.
        return;
    }

    if (m_isFirstExecution)
    {
        m_isFirstExecution = false;

        // Nothing else to do.
    }
    else
    {
        // HierarchyClipboard::Unserialize() takes care of both the editor-side and the runtime-side.
        m_hasPreviouslyFailed = (!HierarchyClipboard::Unserialize(m_hierarchy, m_entryList, isUndo));
    }
}

void CommandPropertiesChange::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    SerializeHelpers::SerializedEntryList& preValueChanges,
    const char* commandName)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandPropertiesChange(stack,
        hierarchy,
        preValueChanges,
        commandName));
}
