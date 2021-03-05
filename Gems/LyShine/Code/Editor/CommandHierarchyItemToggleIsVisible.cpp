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

CommandHierarchyItemToggleIsVisible::CommandHierarchyItemToggleIsVisible(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const HierarchyItemRawPtrList& items)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_ids()
    , m_toIsVisible(false)
{
    setText((items.size() == 1) ? QString("toggle visibility of \"%1\"").arg((*items.begin())->GetElement()->GetName().c_str()) : QString("toggle visibility"));

    for (auto i : items)
    {
        m_ids.push_back(i->GetEntityId());
    }

    EBUS_EVENT_ID_RESULT(m_toIsVisible, m_ids.front(), UiEditorBus, GetIsVisible);
    m_toIsVisible = !m_toIsVisible;
}

void CommandHierarchyItemToggleIsVisible::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsVisible(!m_toIsVisible);
}

void CommandHierarchyItemToggleIsVisible::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetIsVisible(m_toIsVisible);
}

void CommandHierarchyItemToggleIsVisible::SetIsVisible(bool isVisible)
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
        HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(m_hierarchy, element, false))->SetIsVisible(isVisible);
    }
}

void CommandHierarchyItemToggleIsVisible::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const HierarchyItemRawPtrList& items)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemToggleIsVisible(stack,
            hierarchy,
            items));
}
