/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QUndoCommand>

class CommandHierarchyItemCreateFromData
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        bool createAsChildOfSelection,
        HierarchyHelpers::Creator creator,
        const QString& dataSource);

private:

    CommandHierarchyItemCreateFromData(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const EntityHelpers::EntityIdList& parents,
        bool createAsChildOfSelection,
        HierarchyHelpers::Creator creator,
        const QString& dataSource);


    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;
    EntityHelpers::EntityIdList m_parents;
    bool m_createAsChildOfSelection;
    HierarchyHelpers::Creator m_creator;
    SerializeHelpers::SerializedEntryList m_entries;
};
