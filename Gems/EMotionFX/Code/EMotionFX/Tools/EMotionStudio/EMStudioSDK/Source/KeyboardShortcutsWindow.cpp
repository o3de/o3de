/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "KeyboardShortcutsWindow.h"

#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>

#include "EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <EMotionFX/Source/ActorInstance.h>
#include "MainWindow.h"


namespace EMStudio
{
    // constructor
    KeyboardShortcutsWindow::KeyboardShortcutsWindow(QWidget* parent)
        : QWidget(parent)
    {
        m_selectedGroup          = -1;
        m_shortcutReceiverDialog = nullptr;

        // fill the table
        Init();
    }


    // destructor
    KeyboardShortcutsWindow::~KeyboardShortcutsWindow()
    {
    }


    // create the list of parameters
    void KeyboardShortcutsWindow::Init()
    {
        // create the node groups table
        m_tableWidget = new QTableWidget();

        // create the table widget
        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setCornerButtonEnabled(false);
        m_tableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &KeyboardShortcutsWindow::OnShortcutChange);

        // create the list widget
        m_listWidget = new QListWidget();
        m_listWidget->setAlternatingRowColors(true);
        connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &KeyboardShortcutsWindow::OnGroupSelectionChanged);

        // build the layout
        m_hLayout = new QHBoxLayout();
        m_hLayout->setMargin(0);
        m_hLayout->setAlignment(Qt::AlignLeft);

        m_hLayout->addWidget(m_listWidget);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->addWidget(m_tableWidget);

        QLabel* label = new QLabel("Double-click to adjust shortcut");
        label->setAlignment(Qt::AlignCenter);
        vLayout->addWidget(label);
        m_hLayout->addLayout(vLayout);

        // set the main layout
        setLayout(m_hLayout);

        ReInit();

        // automatically select the first entry
        if (m_listWidget->count() > 0)
        {
            m_listWidget->setCurrentRow(0);
        }
    }


    void KeyboardShortcutsWindow::setVisible(bool visible)
    {
        QWidget::setVisible(visible);

        if (visible)
        {
            ReInit();
        }
    }


    // reconstruct the whole interface
    void KeyboardShortcutsWindow::ReInit()
    {
        m_tableWidget->blockSignals(true);

        // clear
        m_listWidget->clear();

        // make the list widget smaller than the table
        m_listWidget->setMinimumWidth(150);
        m_listWidget->setMaximumWidth(150);

        // add the groups to the left list widget
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();
        const size_t numGroups = shortcutManager->GetNumGroups();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            MysticQt::KeyboardShortcutManager::Group* group = shortcutManager->GetGroup(i);
            m_listWidget->addItem(FromStdString(group->GetName()));
        }

        m_tableWidget->blockSignals(false);

        // automatically select the first entry
        m_listWidget->setCurrentRow(m_selectedGroup);
    }


    // if a new keyboard shortcut group got selected this is called
    void KeyboardShortcutsWindow::OnGroupSelectionChanged()
    {
        // get the group index
        m_selectedGroup = m_listWidget->currentRow();
        if (m_selectedGroup == -1)
        {
            return;
        }

        // clear the table
        m_tableWidget->clear();

        // set header item for the table
        m_tableWidget->setColumnCount(2);

        QTableWidgetItem* headerItem = new QTableWidgetItem("Action");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(0, headerItem);

        headerItem = new QTableWidgetItem("Shortcut");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_tableWidget->setHorizontalHeaderItem(1, headerItem);

        // set the vertical header not visible
        QHeaderView* verticalHeader = m_tableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // get access to the shortcut group and some data
        MysticQt::KeyboardShortcutManager*          shortcutManager = GetMainWindow()->GetShortcutManager();
        MysticQt::KeyboardShortcutManager::Group*   group           = shortcutManager->GetGroup(m_selectedGroup);
        const size_t                                numActions      = group->GetNumActions();

        // set the row count
        m_tableWidget->setRowCount(aznumeric_caster(numActions));

        // fill the table with the media root folders
        for (uint32 i = 0; i < numActions; ++i)
        {
            // get the shortcut action
            MysticQt::KeyboardShortcutManager::Action* action = group->GetAction(i);

            // add the item to the table and set the row height
            QTableWidgetItem* item = new QTableWidgetItem(action->m_qaction->text());
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_tableWidget->setItem(i, 0, item);

            const QString keyText = ConstructStringFromShortcut(action->m_qaction->shortcut());

            item = new QTableWidgetItem(keyText);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_tableWidget->setItem(i, 1, item);

            m_tableWidget->setRowHeight(i, 21);
        }

        // resize the first column
        m_tableWidget->resizeColumnToContents(0);

        // needed to have the last column stretching correctly
        m_tableWidget->setColumnWidth(1, 0);

        // set the last column to take the whole space
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    }


    // returns the currently selected group, based on the list widget item on the left
    MysticQt::KeyboardShortcutManager::Group* KeyboardShortcutsWindow::GetCurrentGroup() const
    {
        // get access to the group
        int32 groupIndex = m_listWidget->currentRow();
        if (groupIndex == -1)
        {
            return nullptr;
        }

        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();
        MysticQt::KeyboardShortcutManager::Group* group = shortcutManager->GetGroup(groupIndex);
        return group;
    }


    // called after any change of a shortcut
    void KeyboardShortcutsWindow::OnShortcutChange(int row, int column)
    {
        MCORE_UNUSED(column);

        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        // get access to the group
        MysticQt::KeyboardShortcutManager::Group* group = GetCurrentGroup();

        // get access to the action
        MysticQt::KeyboardShortcutManager::Action* action = group->GetAction(row);

        ShortcutReceiverDialog shortcutWindow(this, action, group);
        m_shortcutReceiverDialog = &shortcutWindow;
        if (shortcutWindow.exec() == QDialog::Accepted)
        {
            // handle conflicts
            if (shortcutWindow.m_conflictDetected)
            {
                shortcutWindow.m_conflictAction->m_qaction->setShortcut({});
            }

            // adjust the shortcut action
            action->m_qaction->setShortcut(shortcutWindow.m_key);

            // save the new shortcuts
            QSettings settings(FromStdString(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioKeyboardShortcuts.cfg")), QSettings::IniFormat, this);
            shortcutManager->Save(&settings);

            // reinit the window
            ReInit();
        }
        m_shortcutReceiverDialog = nullptr;
    }


    // construct a text version of a shortcut
    QString KeyboardShortcutsWindow::ConstructStringFromShortcut(QKeySequence key)
    {
        if (key.isEmpty())
        {
            return "not set";
        }

        return key.toString(QKeySequence::NativeText);
    }


    // reset to default after pressing context menu
    void KeyboardShortcutsWindow::OnResetToDefault()
    {
        if (m_contextMenuAction == nullptr)
        {
            return;
        }

        m_contextMenuAction->m_qaction->setShortcut(m_contextMenuAction->m_defaultKeySequence);

        ReInit();
    }


    // assign a new key after pressing the context menu item
    void KeyboardShortcutsWindow::OnAssignNewKey()
    {
        if (m_contextMenuAction == nullptr)
        {
            return;
        }

        // assign the new shortcut
        OnShortcutChange(m_contextMenuActionIndex, 0);
    }


    // context menu when right clicking on one of the shortcuts
    void KeyboardShortcutsWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // find the table widget item at the clicked position
        QTableWidgetItem* clickedItem = m_tableWidget->itemAt(m_tableWidget->viewport()->mapFromGlobal(event->globalPos()));
        if (clickedItem == nullptr)
        {
            return;
        }

        int actionIndex = clickedItem->row();

        // get access to the group
        MysticQt::KeyboardShortcutManager::Group* group = GetCurrentGroup();

        // get access to the action
        m_contextMenuAction      = group->GetAction(actionIndex);
        m_contextMenuActionIndex = actionIndex;

        // create the context menu
        QMenu menu(this);

        QAction* defaultAction = menu.addAction("Reset To Default");
        connect(defaultAction, &QAction::triggered, this, &KeyboardShortcutsWindow::OnResetToDefault);

        QAction* newKeyAction = menu.addAction("Assign New Key");
        connect(newKeyAction, &QAction::triggered, this, &KeyboardShortcutsWindow::OnAssignNewKey);

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    // constructor
    ShortcutReceiverDialog::ShortcutReceiverDialog(QWidget* parent, MysticQt::KeyboardShortcutManager::Action* action, MysticQt::KeyboardShortcutManager::Group* group)
        : QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
    {
        QVBoxLayout* layout = new QVBoxLayout();

        setObjectName("ShortcutReceiverDialog");

        setWindowTitle(" ");
        layout->addWidget(new QLabel("Press the new shortcut on the keyboard:"));

        m_orgAction          = action;
        m_orgGroup           = group;

        m_conflictAction     = nullptr;
        m_conflictDetected   = false;
        m_key                = action->m_qaction->shortcut();

        QString keyText = KeyboardShortcutsWindow::ConstructStringFromShortcut(m_key);

        m_label = new QLabel(keyText);
        m_label->setAlignment(Qt::AlignHCenter);
        QFont font = m_label->font();
        font.setPointSize(14);
        font.setBold(true);
        m_label->setFont(font);
        layout->addWidget(m_label);

        m_conflictKeyLabel = new QLabel("");
        m_conflictKeyLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(m_conflictKeyLabel);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setMargin(0);

        m_okButton = new QPushButton("OK");
        buttonLayout->addWidget(m_okButton);
        connect(m_okButton, &QPushButton::clicked, this, &ShortcutReceiverDialog::accept);

        QPushButton* defaultButton = new QPushButton("Default");
        buttonLayout->addWidget(defaultButton);
        connect(defaultButton, &QPushButton::clicked, this, &ShortcutReceiverDialog::ResetToDefault);

        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(cancelButton);
        connect(cancelButton, &QPushButton::clicked, this, &ShortcutReceiverDialog::reject);

        layout->addLayout(buttonLayout);

        setLayout(layout);

        setModal(true);
        setWindowModality(Qt::ApplicationModal);
    }


    // reset the shortcut to its default value
    void ShortcutReceiverDialog::ResetToDefault()
    {
        m_key    = m_orgAction->m_defaultKeySequence;

        UpdateInterface();
    }


    // update the shortcut text and other interface elements
    void ShortcutReceiverDialog::UpdateInterface()
    {
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        // check if the currently assigned shortcut is already taken by another shortcut
        m_conflictAction = shortcutManager->FindShortcut(m_key, m_orgGroup);
        if (m_conflictAction == nullptr || m_conflictAction == m_orgAction)
        {
            m_okButton->setToolTip("");
            m_label->setStyleSheet("");
            m_conflictKeyLabel->setStyleSheet("");
            m_conflictKeyLabel->setText("");
            m_conflictDetected = false;
        }
        else
        {
            m_label->setStyleSheet("color: rgb(244, 156, 28);");
            m_conflictKeyLabel->setStyleSheet("color: rgb(244, 156, 28);");

            m_conflictDetected = true;

            if (m_conflictAction)
            {
                m_okButton->setToolTip(QString("Assigning new shortcut will unassign '%1' automatically.").arg(m_conflictAction->m_qaction->text()));

                MysticQt::KeyboardShortcutManager::Group* conflictGroup = shortcutManager->FindGroupForShortcut(m_conflictAction);
                if (conflictGroup)
                {
                    m_conflictKeyLabel->setText(QString("Conflicts with: %1 -> %2").arg(FromStdString(conflictGroup->GetName())).arg(m_conflictAction->m_qaction->text()));
                }
                else
                {
                    m_conflictKeyLabel->setText(QString("Conflicts with: %1").arg(m_conflictAction->m_qaction->text()));
                }
            }
        }

        // adjust the label text to the new shortcut
        const QString keyText = KeyboardShortcutsWindow::ConstructStringFromShortcut(m_key);
        m_label->setText(keyText);
    }

    // called when the user pressed a new shortcut
    void ShortcutReceiverDialog::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() ==  Qt::Key_Alt ||
            event->key() ==  Qt::Key_AltGr ||
            event->key() ==  Qt::Key_Shift ||
            event->key() ==  Qt::Key_Control ||
            event->key() ==  Qt::Key_Meta ||
            event->key() ==  Qt::Key_Tab)
        {
            return;
        }

        if (event->key() == Qt::Key_Escape)
        {
            // close the dialog when pressing ESC
            reject();
        }
        else
        {
            m_key = event->key() | event->modifiers();
        }

        UpdateInterface();

        event->accept();
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_KeyboardShortcutsWindow.cpp>
