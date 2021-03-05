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

CommandHierarchyItemToggleIsExpanded::CommandHierarchyItemToggleIsExpanded(UndoStack* stack,
    HierarchyWidget* hierarchy,
    HierarchyItem* item)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_id(item->GetEntityId())
    , m_toIsExpanded(item->isExpanded())
{
    setText(QString("%1 of \"%2\"").arg((m_toIsExpanded ? "expansion" : "collapse"), item->GetElement()->GetName().c_str()));
}

void CommandHierarchyItemToggleIsExpanded::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsExpanded(!m_toIsExpanded);
}

void CommandHierarchyItemToggleIsExpanded::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsExpanded(m_toIsExpanded);
}

void CommandHierarchyItemToggleIsExpanded::SetIsExpanded(bool isExpanded)
{
    AZ::Entity* element = EntityHelpers::GetEntity(m_id);
    if (!element)
    {
        // The element DOESN'T exist.
        // Nothing to do.
        return;
    }

    // This will do both the Runtime-side and Editor-side.
    HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(m_hierarchy, element, false))->SetIsExpanded(isExpanded);
}

void CommandHierarchyItemToggleIsExpanded::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    HierarchyItem* item)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemToggleIsExpanded(stack,
            hierarchy,
            item));
}
