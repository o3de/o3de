/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : For listing available script commands with their descriptions


#ifndef CRYINCLUDE_EDITOR_SCRIPTHELPDIALOG_H
#define CRYINCLUDE_EDITOR_SCRIPTHELPDIALOG_H
#pragma once

#include <AzCore/Debug/Trace.h>

#if !defined(Q_MOC_RUN)
#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVector>
#include <QMouseEvent>
#include <QHeaderView>
#include <QScopedPointer>
#endif

class QResizeEvent;

namespace Ui {
    class ScriptDialog;
}

namespace AzToolsFramework
{
    class HeaderView
        : public QHeaderView
    {
        Q_OBJECT
    public:
        explicit HeaderView(QWidget* parent = nullptr);
        QSize sizeHint() const override;
    Q_SIGNALS:
        void commandFilterChanged(const QString& text);
        void moduleFilterChanged(const QString& text);
        void descriptionFilterChanged(const QString& text);
        void exampleFilterChanged(const QString& text);
    private Q_SLOTS:
        void repositionLineEdits();
    protected:
        void resizeEvent(QResizeEvent* ev) override;
    private:
        QLineEdit* const m_commandFilter;
        QLineEdit* const m_moduleFilter;
        QLineEdit* const m_descriptionFilter;
        QLineEdit* const m_exampleFilter;
        int m_lineEditHeightOffset;
    };

    class ScriptHelpProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        explicit ScriptHelpProxyModel(QObject* parent = nullptr);
        bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

        void setCommandFilter(const QString&);
        void setModuleFilter(const QString&);
        void setDescriptionFilter(const QString& text);
        void setExampleFilter(const QString& text);
    private:
        QString m_commandFilter;
        QString m_moduleFilter;
        QString m_descriptionFilter;
        QString m_exampleFilter;
    };

    class ScriptHelpModel
        : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        enum Column
        {
            ColumnCommand,
            ColumnModule,
            ColumnDescription,
            ColumnCount // keep at end, for iteration purposes
        };

        struct Item
        {
            QString command;
            QString module;
            QString description;
            QString example;
        };
        typedef QVector<Item> Items;


        explicit ScriptHelpModel(QObject* parent = nullptr);
        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex & = {}) const override;
        int columnCount(const QModelIndex & = {}) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        void Reload();

    private:
        Items m_items;
    };

    class ScriptTableView
        : public QTableView
    {
        Q_OBJECT
    public:
        explicit ScriptTableView(QWidget* parent = nullptr);

    private:
        ScriptHelpModel* const m_model;
        ScriptHelpProxyModel* const m_proxyModel;
    };

    class CScriptHelpDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        static CScriptHelpDialog* GetInstance()
        {
            static CScriptHelpDialog* pInstance = nullptr;
            if (!pInstance)
            {
                QMainWindow* mainWindow = GetMainWindowOfCurrentApplication();
                if (!mainWindow)
                {
                    AZ_Assert(false, "Failed to find MainWindow.");
                    return nullptr;
                }

                QWidget* parentWidget = mainWindow->window() ? mainWindow->window() : mainWindow; // MainWindow might have a WindowDecorationWrapper parent. Makes a difference on macOS.
                pInstance = new CScriptHelpDialog(parentWidget);
            }
            return pInstance;
        }

    private Q_SLOTS:
        void OnDoubleClick(const QModelIndex&);

    private:
        static QMainWindow* GetMainWindowOfCurrentApplication()
        {
            QMainWindow* mainWindow = nullptr;
            for (QWidget* w : qApp->topLevelWidgets())
            {
                mainWindow = qobject_cast<QMainWindow*>(w);
                if (mainWindow)
                {
                    return mainWindow;
                }
            }
            return nullptr;
        }

        explicit CScriptHelpDialog(QWidget* parent = nullptr);
        QScopedPointer<Ui::ScriptDialog> ui;
    };
} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_SCRIPTHELPDIALOG_H
