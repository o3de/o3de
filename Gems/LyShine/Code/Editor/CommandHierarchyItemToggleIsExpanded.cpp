/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
