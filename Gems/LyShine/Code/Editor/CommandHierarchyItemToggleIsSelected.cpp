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
