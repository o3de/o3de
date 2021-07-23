/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QUndoCommand>

class CommandHierarchyItemDelete
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const QTreeWidgetItemRawPtrQList& selectedItems);

private:

    CommandHierarchyItemDelete(UndoStack* stack,
        HierarchyWidget* hierarchy);

    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;

    SerializeHelpers::SerializedEntryList m_entries;
};
