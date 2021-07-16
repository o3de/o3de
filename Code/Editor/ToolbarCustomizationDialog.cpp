/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ToolbarCustomizationDialog.h"

// Qt
#include <QMenuBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>

// AzQtComponents
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

// Editor
#include "MainWindow.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_ToolbarCustomizationDialog.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

using namespace AzQtComponents;

enum ItemDataRole
{
    ToolbarRole = Qt::UserRole,
    ToolbarNameRole
};

enum Tab
{
    TabToolbars,
    TabCommands,
    TabOptions,
    TabKeyboard
};

ToolbarCustomizationDialog::ToolbarCustomizationDialog(MainWindow* mainWindow)
    : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach | WindowDecorationWrapper::OptionAutoTitleBarButtons, mainWindow))
    , ui(new Ui::ToolbarCustomizationDialog())
    , m_mainWindow(mainWindow)
    , m_toolbarManager(mainWindow->GetToolbarManager())
{
    m_toolbarManager->SetIsEditingToolBars(true);
    ui->setupUi(this);
    setAcceptDrops(true);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    connect(ui->closeButton, &QPushButton::clicked, this, &QWidget::close);
    Setup();
    setAttribute(Qt::WA_DeleteOnClose);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &ToolbarCustomizationDialog::OnTabChanged);
    connect(ui->newButton, &QPushButton::clicked, this, [this] {
        NewToolbar();
    });
    connect(ui->renameButton, &QPushButton::clicked, this, &ToolbarCustomizationDialog::RenameToolbar);
    connect(ui->deleteButton, &QPushButton::clicked, this, &ToolbarCustomizationDialog::DeleteToolbar);
    connect(ui->resetButton, &QPushButton::clicked, this, &ToolbarCustomizationDialog::ResetToolbar);

    ui->commandsListWidget->setDragDropMode(QAbstractItemView::DragOnly);
    ui->commandsListWidget->setDragEnabled(true);

    // Hide the options and keyboard tabs. TODO: See what amazon wants to do with these. Most options don't make sense for Qt.
    ui->tabWidget->removeTab(3);
    ui->tabWidget->removeTab(2);
}

ToolbarCustomizationDialog::~ToolbarCustomizationDialog()
{
    m_toolbarManager->SetIsEditingToolBars(false);
}

void ToolbarCustomizationDialog::OnTabChanged(int index)
{
    switch (static_cast<Tab>(index))
    {
    case TabToolbars:
        ui->toolbarsListWidget->setFocus(Qt::OtherFocusReason);
        break;
    case TabCommands:
        ui->categoriesListWidget->setFocus(Qt::OtherFocusReason);
        break;
    case TabOptions:
        break;
    case TabKeyboard:
        ui->keyboardCommandsListWidget->setFocus(Qt::OtherFocusReason);
        break;
    }
}

QList<QAction*> ToolbarCustomizationDialog::toplevelActions() const
{
    return m_mainWindow->menuBar()->actions();
}

void ToolbarCustomizationDialog::Setup()
{
    SetupCategoryCombo();
    SetupCategoryListWidget();
    SetupToolbarsListWidget();
}

void ToolbarCustomizationDialog::SetupCategoryCombo()
{
    ui->categoryCombo->clear();
    for (QAction* action : toplevelActions())
    {
        ui->categoryCombo->addItem(action->text().remove(QLatin1Char('&')));
    }

    connect(ui->categoryCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ToolbarCustomizationDialog::FillKeyboardCommands, Qt::UniqueConnection);

    FillKeyboardCommands();
}

void ToolbarCustomizationDialog::FillKeyboardCommands()
{
    ui->keyboardCommandsListWidget->clear();
    int idx = ui->categoryCombo->currentIndex();

    QList<QAction*> rootActions = toplevelActions();
    if (idx < 0 || idx >= rootActions.size())
    {
        return;
    }

    QAction* action = rootActions.at(idx);

    QMenu* menu = action->menu();
    if (!menu)
    {
        return;
    }

    for (QAction* menuAction : menu->actions())
    {
        if (!menuAction->isSeparator())
        {
            ui->keyboardCommandsListWidget->addItem(menuAction->text().remove(QLatin1Char('&')));
        }
    }
}

void ToolbarCustomizationDialog::SetupCategoryListWidget()
{
    ui->categoriesListWidget->clear();
    for (QAction* action : toplevelActions())
    {
        ui->categoriesListWidget->addItem(action->text().remove(QLatin1Char('&')));
    }

    QModelIndex firstRow = ui->categoriesListWidget->model()->index(0, 0);
    ui->categoriesListWidget->selectionModel()->select(firstRow, QItemSelectionModel::Select);

    FillCommandsListWidget();
    connect(ui->categoriesListWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &ToolbarCustomizationDialog::FillCommandsListWidget);
}

void ToolbarCustomizationDialog::FillCommandsListWidget()
{
    ui->commandsListWidget->clear();
    QModelIndexList indexes = ui->categoriesListWidget->selectionModel()->selectedIndexes();
    QModelIndex index = indexes.isEmpty() ? QModelIndex() : indexes.first();
    if (!index.isValid())
    {
        return;
    }

    const int row = index.row();
    const QList<QAction*> rootActions = toplevelActions();
    if (row < 0 || row >= rootActions.size())
    {
        return;
    }

    QMenu* menu = rootActions[row]->menu();
    if (!menu)
    {
        return;
    }

    for (QAction* action : menu->actions())
    {
        if (!action->isSeparator() && !action->menu())
        {
            auto item = new QListWidgetItem(action->text().remove(QLatin1Char('&')));
            auto icon = action->icon();
            const int actionId = action->data().toInt();
            item->setData(ActionRole, actionId);
            if (!icon.isNull())
            {
                item->setIcon(QIcon(icon.pixmap(32).scaled(QSize(16, 16))));
            }

            ui->commandsListWidget->addItem(item);
        }
    }
}

void ToolbarCustomizationDialog::SetupToolbarsListWidget()
{
    ui->toolbarsListWidget->clear();
    auto toolbars = m_toolbarManager->GetToolbars();
    connect(ui->toolbarsListWidget, &QListWidget::itemChanged,
        this, &ToolbarCustomizationDialog::ToggleToolbar, Qt::UniqueConnection);
    connect(ui->toolbarsListWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &ToolbarCustomizationDialog::OnToolbarSelected, Qt::UniqueConnection);
    for (const AmazonToolbar& at : toolbars)
    {
        AddToolbarItem(at);
    }
    OnToolbarSelected(); // initial button state
}

void ToolbarCustomizationDialog::OnToolbarSelected()
{
    auto indexes = ui->toolbarsListWidget->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
    {
        int row = indexes.at(0).row();
        const bool isValid = row >= 0;
        const bool isCustom = m_toolbarManager->IsCustomToolbar(row);
        const bool isStandard = isValid && !isCustom;

        ui->newButton->setEnabled(isValid);
        ui->resetButton->setEnabled(isStandard);
        ui->renameButton->setEnabled(isCustom);
        ui->deleteButton->setEnabled(isCustom);
    }
    else
    {
        ui->resetButton->setEnabled(false);
        ui->renameButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);
    }
}

void ToolbarCustomizationDialog::AddToolbarItem(const AmazonToolbar& at, bool forceVisible)
{
    auto item = new QListWidgetItem();
    item->setData(ToolbarRole, QVariant::fromValue<QToolBar*>(at.Toolbar()));
    item->setData(ToolbarNameRole, at.GetName());
    item->setText(at.Toolbar()->windowTitle());
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
    item->setCheckState((forceVisible || at.Toolbar()->isVisible()) ? Qt::Checked : Qt::Unchecked);
    ui->toolbarsListWidget->addItem(item);
}

void ToolbarCustomizationDialog::ToggleToolbar(QListWidgetItem* item)
{
    auto toolbar = item->data(ToolbarRole).value<QToolBar*>();
    toolbar->setVisible(item->checkState() == Qt::Checked);
}

void ToolbarCustomizationDialog::NewToolbar(const QString &initialName)
{
    QString name = QInputDialog::getText(this, tr("New Toolbar"), tr("Toolbar name:"), QLineEdit::Normal, initialName);
    for (const AmazonToolbar &t : m_toolbarManager->GetToolbars()) {
        if (name == t.GetTranslatedName()) {
            QMessageBox::warning(this, tr("Warning"),
                tr("A toolbar with this name already exists. Please choose a different name."),
                QMessageBox::Ok);
            NewToolbar(name);
            return;
        }
    }
    int index = m_toolbarManager->Add(name);
    if (index != -1)
    {
        AddToolbarItem(m_toolbarManager->GetToolbar(index), /**forceVisible=*/ true); // toolbar is only visible in the next event loop, so check the checkbox anyway
    }
}

void ToolbarCustomizationDialog::DeleteToolbar()
{
    QModelIndex selectedIndex = SelectedToolbarIndex();
    if (!selectedIndex.isValid())
    {
        return;
    }

    QString name = selectedIndex.data(Qt::DisplayRole).toString();

    if (QMessageBox::question(this, "Editor", QString("Are you sure you want to delete the '%1' toolbar?").arg(name)) == QMessageBox::Yes)
    {
        const int index = SelectedToolbarIndex().row();
        if (m_toolbarManager->Delete(index))
        {
            ui->toolbarsListWidget->selectionModel()->clear();
            delete ui->toolbarsListWidget->item(index);
        }
    }
}

void ToolbarCustomizationDialog::RenameToolbar()
{
    const int index = SelectedToolbarIndex().row();
    if (QListWidgetItem* item = ui->toolbarsListWidget->item(index))
    {
        const QString currentName = item->data(Qt::DisplayRole).toString();
        const QString newName = QInputDialog::getText(this, tr("Rename Toolbar"), tr("Toolbar name:"),
                QLineEdit::Normal, currentName);
        if (m_toolbarManager->Rename(SelectedToolbarIndex().row(), newName))
        {
            item->setData(Qt::DisplayRole, newName);
        }
    }
}

void ToolbarCustomizationDialog::ResetToolbar()
{
    QModelIndex selectedIndex = SelectedToolbarIndex();
    if (!selectedIndex.isValid())
    {
        return;
    }

    QString displayName = selectedIndex.data(Qt::DisplayRole).toString();

    if (QMessageBox::question(this, "Editor", QString("Are you sure you want to reset the changes made to the '%1' toolbar?").arg(displayName)) == QMessageBox::Yes)
    {
        QString toolbarName = selectedIndex.data(ToolbarNameRole).toString();
        m_toolbarManager->RestoreToolbarDefaults(toolbarName);
    }
}

QModelIndex ToolbarCustomizationDialog::SelectedToolbarIndex() const
{
    auto indexes = ui->toolbarsListWidget->selectionModel()->selectedIndexes();
    return indexes.isEmpty() ? QModelIndex() : indexes.at(0);
}

void ToolbarCustomizationDialog::dragEnterEvent(QDragEnterEvent* ev)
{
    ev->accept();
}

void ToolbarCustomizationDialog::dragMoveEvent(QDragMoveEvent* ev)
{
    ev->accept();
}

void ToolbarCustomizationDialog::dropEvent(QDropEvent* ev)
{
    auto sourceWidget = qobject_cast<QWidget*>(ev->source());
    EditableQToolBar* sourceToolbar = m_toolbarManager->ToolbarParent(sourceWidget);

    if (!sourceToolbar || !sourceWidget)
    {
        // Doesn't happen
        qWarning() << Q_FUNC_INFO << "Invalid source widget or toolbar" << sourceWidget << sourceToolbar;
        return;
    }

    QAction* action = sourceToolbar->ActionForWidget(sourceWidget);
    const int actionId = action ? action->data().toInt() : 0;
    if (actionId <= 0)
    {
        // Doesn't happen
        qWarning() << Q_FUNC_INFO << "Invalid action id" << actionId << sourceWidget;
        return;
    }

    m_toolbarManager->DeleteAction(action, sourceToolbar);
}

#include <moc_ToolbarCustomizationDialog.cpp>
