/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

CommandHierarchyItemToggleIsSelected::CommandHierarchyItemToggleIsSelected(UndoStack* stack,
    HierarchyWidget* hierarchy,
    HierarchyItem* item)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_id(item->GetEntityId())
    , m_toIsSelected(false)
{
    setText(QString("toggle selection of \"%1\"").arg(item->GetElement()->GetName().c_str()));

    EBUS_EVENT_ID_RESULT(m_toIsSelected, m_id, UiEditorBus, GetIsSelected);
    m_toIsSelected = !m_toIsSelected;
}

void CommandHierarchyItemToggleIsSelected::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsSelected(!m_toIsSelected);
}

void CommandHierarchyItemToggleIsSelected::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsSelected(m_toIsSelected);
}

void CommandHierarchyItemToggleIsSelected::SetIsSelected(bool isSelected)
{
    AZ::Entity* element = EntityHelpers::GetEntity(m_id);
    if (!element)
    {
        // The element DOESN'T exist.
        // Nothing to do.
        return;
    }

    // This will do both the Runtime-side and Editor-side.
    HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(m_hierarchy, element, false))->SetIsSelected(isSelected);
}

void CommandHierarchyItemToggleIsSelected::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    HierarchyItem* item)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemToggleIsSelected(stack,
            hierarchy,
            item));
}
