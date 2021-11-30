/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

CommandHierarchyItemRename::CommandHierarchyItemRename(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const AZ::EntityId& entityId,
    const QString& fromName,
    const QString& toName)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_id(entityId)
    , m_from(fromName)
    , m_to(toName)
{
    setText(QString("rename to \"%1\"").arg(m_to));
}

void CommandHierarchyItemRename::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetName(m_from);
}

void CommandHierarchyItemRename::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetName(m_to);
}

void CommandHierarchyItemRename::SetName(const QString& name) const
{
    // Runtime-side.
    AZ::Entity* element = EntityHelpers::GetEntity(m_id);
    {
        if (!element)
        {
            // The element DOESN'T exist.
            // Nothing to do.
            return;
        }

        element->SetName(name.toStdString().c_str());
    }

    m_hierarchy->GetEditorWindow()->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);

    // Editor-side.
    //
    // IMPORTANT: It's NOT necessary to prevent this from executing
    // on the first run. We WON'T get a redundant Qt notification.
    HierarchyHelpers::ElementToItem(m_hierarchy, element, false)->setText(0, name);
}

void CommandHierarchyItemRename::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const AZ::EntityId& entityId,
    const QString& fromName,
    const QString& toName)
{
    // IMPORTANT: Using QSignalBlocker with hierarchy->model() here
    // DOESN'T prevent multiple notifications. Therefore we HAVE to
    // filter-out redundant notifications manually.
    if (fromName == toName)
    {
        // Both strings are the same.
        // Nothing to do.
        return;
    }

    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemRename(stack,
            hierarchy,
            entityId,
            fromName,
            toName));
}
