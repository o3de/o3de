/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractItemView>
#include <QLineEdit>
#include <QRect>
#include <QPoint>
#include <QString>

#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will write a string to the specified line edit.
    */
    class WriteToLineEditAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(WriteToLineEditAction, AZ::SystemAllocator);
        AZ_RTTI(WriteToLineEditAction, "{EAF64BDE-9A06-4169-8FD1-50D6BFEBA73A}", CompoundAction);

        WriteToLineEditAction(QLineEdit* targetEdit, QString targetText);

        void SetupAction() override;

    private:

        QLineEdit* m_targetEdit;
        QString m_targetText;
    };

    /**
        EditorAutomationAction that will move the mouse to the specified row for the specified ItemView
    */
    class MoveMouseToViewRowAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(MoveMouseToViewRowAction, AZ::SystemAllocator);
        AZ_RTTI(MoveMouseToViewRowAction, "{094DD153-BDE4-4E56-B14A-25FE5C6348A1}", CompoundAction);

        MoveMouseToViewRowAction(QAbstractItemView* tableView, int row, QModelIndex parentIndex = QModelIndex());
        ~MoveMouseToViewRowAction() override = default;

        void SetupAction() override;

    private:

        QAbstractItemView* m_itemView = nullptr;
        int m_row;
        QModelIndex m_parentIndex;
    };
}
