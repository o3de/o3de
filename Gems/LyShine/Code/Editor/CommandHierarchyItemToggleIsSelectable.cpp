/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

CommandHierarchyItemToggleIsSelectable::CommandHierarchyItemToggleIsSelectable(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const HierarchyItemRawPtrList& items)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_ids()
    , m_toIsSelectable(false)
{
    setText((items.size() == 1) ? QString("toggle selectability of \"%1\"").arg((*items.begin())->GetElement()->GetName().c_str()) : QString("toggle selectability"));

    for (auto i : items)
    {
        m_ids.push_back(i->GetEntityId());
    }

    UiEditorBus::EventResult(m_toIsSelectable, m_ids.front(), &UiEditorBus::Events::GetIsSelectable);
    m_toIsSelectable = !m_toIsSelectable;
}

void CommandHierarchyItemToggleIsSelectable::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsSelectable(!m_toIsSelectable);
}

void CommandHierarchyItemToggleIsSelectable::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsSelectable(m_toIsSelectable);
}

void CommandHierarchyItemToggleIsSelectable::SetIsSelectable(bool isSelectable)
{
    if (!HierarchyHelpers::AllItemExists(m_hierarchy, m_ids))
    {
        // At least one item is missing.
        // Nothing to do.
        return;
    }

    for (auto i : m_ids)
    {
        AZ::Entity* element = EntityHelpers::GetEntity(i);

        // This will do both the Runtime-side and Editor-side.
        HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(m_hierarchy, element, false))->SetIsSelectable(isSelectable);
    }
}

void CommandHierarchyItemToggleIsSelectable::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const HierarchyItemRawPtrList& items)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemToggleIsSelectable(stack,
            hierarchy,
            items));
}
