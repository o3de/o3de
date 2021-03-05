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
