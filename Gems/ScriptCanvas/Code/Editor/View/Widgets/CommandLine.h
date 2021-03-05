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

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#endif

namespace Ui
{
    class CommandLine;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        // TODO #lsempe: this deserves its own file
        // CommandListDataModel
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandListDataModel : public QAbstractTableModel
        {
            Q_OBJECT

        public:

            AZ_CLASS_ALLOCATOR(CommandListDataModel, AZ::SystemAllocator, 0);

            enum ColumnIndex
            {
                Command,
                Description,
                Trail,
                Count
            };

            enum CustomRole
            {
                Node = Qt::UserRole,
                Types,
                EBusSender,
                EBusHandler,
                Commands
            };

            CommandListDataModel(QWidget* parent = nullptr);
            QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
            QModelIndex parent(const QModelIndex &child) const override;
            int rowCount(const QModelIndex &parent = QModelIndex()) const override;
            int columnCount(const QModelIndex &parent = QModelIndex()) const override;
            QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

            Qt::ItemFlags flags(const QModelIndex &index) const override;

            bool HasMatches(const AZStd::string& input);

        protected:

            AZStd::vector<AZ::Uuid> m_nodeTypes;

        };

        class CommandListDataProxyModel : public QSortFilterProxyModel
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CommandListDataProxyModel, AZ::SystemAllocator, 0);

            CommandListDataProxyModel(QObject* parent = nullptr);

            bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

            void SetInput(const AZStd::string& input)
            {
                m_input = input;
                invalidate();
            }

        protected:
            QCompleter* m_completer;
            AZStd::string m_input;

        };


        // CommandLineEdit
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandLineEdit : public QLineEdit
        {
            Q_OBJECT
        public:
            CommandLineEdit(QWidget* parent = nullptr);
            void ResetState();

        Q_SIGNALS:
            void onFocusChange(bool focused);
            void onKeyReleased(QKeyEvent*);

        protected:
            void focusInEvent(QFocusEvent*) override;
            void focusOutEvent(QFocusEvent*) override;
            void keyPressEvent(QKeyEvent *) override;
            void keyReleaseEvent(QKeyEvent*) override;

            void onTextChanged(const QString&);
            void onTextEdited(const QString&);
            void onReturnPressed();

            bool m_empty;
            const QString m_defaultText;
        };

        // CommandLineList
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandLineList : public QTableView
        {
            Q_OBJECT
        public:
            CommandLineList(QWidget* parent = nullptr);

        Q_SIGNALS:
            void onKeyReleased(QKeyEvent*);

        protected:
            void keyReleaseEvent(QKeyEvent* event) override
            {
                Q_EMIT(onKeyReleased(event));
            }
        };

        // CommandLine
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandLine 
            : public QWidget
        {
            Q_OBJECT
        public:

            CommandLine(QWidget* object = nullptr);

            void showEvent(QShowEvent *event) override;
            void onTextChanged(const QString&);
            void onEditKeyReleaseEvent(QKeyEvent*);
            void onListKeyReleaseEvent(QKeyEvent*);

            AZStd::unique_ptr<Ui::CommandLine> ui;
        };
    }
}