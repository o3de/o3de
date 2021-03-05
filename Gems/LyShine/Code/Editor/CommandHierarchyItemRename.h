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

class CommandHierarchyItemRename
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const AZ::EntityId& entityId,
        const QString& fromName,
        const QString& toName);

private:

    CommandHierarchyItemRename(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const AZ::EntityId& entityId,
        const QString& fromName,
        const QString& toName);

    void SetName(const QString& name) const;

    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;
    AZ::EntityId m_id;
    QString m_from;
    QString m_to;
};
