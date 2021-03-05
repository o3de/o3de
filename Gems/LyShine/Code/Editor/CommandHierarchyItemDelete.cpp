/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#include <Animation/UiEditorAnimationBus.h>

CommandHierarchyItemDelete::CommandHierarchyItemDelete(UndoStack* stack,
    HierarchyWidget* hierarchy)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_entries()
{
    // true: Put the serialized data in m_undoXml.
    HierarchyClipboard::Serialize(m_hierarchy, m_hierarchy->selectedItems(), nullptr, m_entries, true);
    AZ_Assert(!m_entries.empty(), "Failed to serialize");

    setText(QString("delete element") + (m_entries.empty() ? "" : "s"));
}

void CommandHierarchyItemDelete::undo()
{
    UndoStackExecutionScope s(m_stack);

    HierarchyHelpers::CreateItemsAndElements(m_hierarchy, m_entries);

    EBUS_EVENT(UiEditorAnimListenerBus, OnUiElementsDeletedOrReAdded);
}

void CommandHierarchyItemDelete::redo()
{
    UndoStackExecutionScope s(m_stack);

    HierarchyHelpers::Delete(m_hierarchy, m_entries);

    EBUS_EVENT(UiEditorAnimListenerBus, OnUiElementsDeletedOrReAdded);
}

void CommandHierarchyItemDelete::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const QTreeWidgetItemRawPtrQList& selectedItems)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    if (selectedItems.empty())
    {
        // Nothing selected. Nothing to do.
        return;
    }

    stack->push(new CommandHierarchyItemDelete(stack, hierarchy));
}
