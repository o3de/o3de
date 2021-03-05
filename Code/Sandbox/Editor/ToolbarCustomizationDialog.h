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
#ifndef TOOLBAR_CUSTOMIZATION_DIALOG_H
#define TOOLBAR_CUSTOMIZATION_DIALOG_H

#if !defined(Q_MOC_RUN)
#include "ToolbarManager.h"

#include <QDialog>
#include <QScopedPointer>
#include <QList>
#endif

class ToolbarManager;
class MainWindow;
class QAction;
class QListWidgetItem;
class QModelIndex;
class QDropEvent;

namespace Ui {
    class ToolbarCustomizationDialog;
}

class ToolbarCustomizationDialog
    : public QDialog
{
    Q_OBJECT
public:
    explicit ToolbarCustomizationDialog(MainWindow* parent = nullptr);
    ~ToolbarCustomizationDialog();

protected:
    void dragMoveEvent(QDragMoveEvent* ev) override;
    void dragEnterEvent(QDragEnterEvent* ev) override;
    void dropEvent(QDropEvent* ev);

private:
    void OnTabChanged(int index);
    QList<QAction*> toplevelActions() const;
    void ToggleToolbar(QListWidgetItem*);
    void Setup();
    void SetupCategoryCombo();
    void SetupCategoryListWidget();
    void SetupToolbarsListWidget();
    void FillKeyboardCommands();
    void FillCommandsListWidget();
    QModelIndex SelectedToolbarIndex() const;
    void AddToolbarItem(const AmazonToolbar&, bool forceVisible = false);
    void OnToolbarSelected();

    void NewToolbar(const QString &initialName = QString());
    void DeleteToolbar();
    void RenameToolbar();
    void ResetToolbar();

    QScopedPointer<Ui::ToolbarCustomizationDialog> ui;
    MainWindow* const m_mainWindow;
    ToolbarManager* const m_toolbarManager;
};

#endif
