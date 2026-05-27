/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <QString>
#include <QUndoCommand>

class UndoStack;
class HierarchyWidget;

class CommandHierarchyItemRename : public QUndoCommand
{
public:
    void undo() override;
    void redo() override;

    static void Push(
        UndoStack* stack, HierarchyWidget* hierarchy, const AZ::EntityId& entityId, const QString& fromName, const QString& toName);

private:
    CommandHierarchyItemRename(
        UndoStack* stack, HierarchyWidget* hierarchy, const AZ::EntityId& entityId, const QString& fromName, const QString& toName);

    void SetName(const QString& name) const;

private:
    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;
    AZ::EntityId m_id;
    QString m_from;
    QString m_to;
};
