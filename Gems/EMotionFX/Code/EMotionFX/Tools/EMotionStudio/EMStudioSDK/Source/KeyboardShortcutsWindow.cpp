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
        mSelectedGroup          = -1;
        mShortcutReceiverDialog = nullptr;

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
        mTableWidget = new QTableWidget();

        // create the table widget
        mTableWidget->setSortingEnabled(false);
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

        connect(mTableWidget, &QTableWidget::cellDoubleClicked, this, &KeyboardShortcutsWindow::OnShortcutChange);

        // create the list widget
        mListWidget = new QListWidget();
        mListWidget->setAlternatingRowColors(true);
        connect(mListWidget, &QListWidget::itemSelectionChanged, this, &KeyboardShortcutsWindow::OnGroupSelectionChanged);

        // build the layout
        mHLayout = new QHBoxLayout();
        mHLayout->setMargin(0);
        mHLayout->setAlignment(Qt::AlignLeft);

        mHLayout->addWidget(mListWidget);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->addWidget(mTableWidget);

        QLabel* label = new QLabel("Double-click to adjust shortcut");
        label->setAlignment(Qt::AlignCenter);
        vLayout->addWidget(label);
        mHLayout->addLayout(vLayout);

        // set the main layout
        setLayout(mHLayout);

        ReInit();

        // automatically select the first entry
        if (mListWidget->count() > 0)
        {
            mListWidget->setCurrentRow(0);
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


    void KeyboardShortcutsWindow::hideEvent(QHideEvent* event)
    {
        MCORE_UNUSED(event);
        //if (mShortcutReceiverDialog)
        //  mShortcutReceiverDialog->reject();
    }


    // reconstruct the whole interface
    void KeyboardShortcutsWindow::ReInit()
    {
        mTableWidget->blockSignals(true);

        // clear
        mListWidget->clear();

        // make the list widget smaller than the table
        mListWidget->setMinimumWidth(150);
        mListWidget->setMaximumWidth(150);

        // add the groups to the left list widget
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();
        const uint32 numGroups = shortcutManager->GetNumGroups();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            MysticQt::KeyboardShortcutManager::Group* group = shortcutManager->GetGroup(i);
            mListWidget->addItem(group->GetName());
        }

        mTableWidget->blockSignals(false);

        // automatically select the first entry
        mListWidget->setCurrentRow(mSelectedGroup);
    }


    // if a new keyboard shortcut group got selected this is called
    void KeyboardShortcutsWindow::OnGroupSelectionChanged()
    {
        // get the group index
        mSelectedGroup = mListWidget->currentRow();
        if (mSelectedGroup == -1)
        {
            return;
        }

        // clear the table
        mTableWidget->clear();

        // set header item for the table
        mTableWidget->setColumnCount(2);

        QTableWidgetItem* headerItem = new QTableWidgetItem("Action");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(0, headerItem);

        headerItem = new QTableWidgetItem("Shortcut");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(1, headerItem);

        // set the vertical header not visible
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // get access to the shortcut group and some data
        MysticQt::KeyboardShortcutManager*          shortcutManager = GetMainWindow()->GetShortcutManager();
        MysticQt::KeyboardShortcutManager::Group*   group           = shortcutManager->GetGroup(mSelectedGroup);
        const uint32                                numActions      = group->GetNumActions();

        // set the row count
        mTableWidget->setRowCount(numActions);

        // fill the table with the media root folders
        for (uint32 i = 0; i < numActions; ++i)
        {
            // get the shortcut action
            MysticQt::KeyboardShortcutManager::Action* action = group->GetAction(i);

            // add the item to the table and set the row height
            QTableWidgetItem* item = new QTableWidgetItem(action->mName.c_str());
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            mTableWidget->setItem(i, 0, item);

            const QString keyText = ConstructStringFromShortcut(action->mKey, action->mCtrl, action->mAlt);

            item = new QTableWidgetItem(keyText);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            mTableWidget->setItem(i, 1, item);

            mTableWidget->setRowHeight(i, 21);
        }

        // resize the first column
        mTableWidget->resizeColumnToContents(0);

        // needed to have the last column stretching correctly
        mTableWidget->setColumnWidth(1, 0);

        // set the last column to take the whole space
        mTableWidget->horizontalHeader()->setStretchLastSection(true);
    }


    // returns the currently selected group, based on the list widget item on the left
    MysticQt::KeyboardShortcutManager::Group* KeyboardShortcutsWindow::GetCurrentGroup() const
    {
        // get access to the group
        int32 groupIndex = mListWidget->currentRow();
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
        mShortcutReceiverDialog = &shortcutWindow;
        if (shortcutWindow.exec() == QDialog::Accepted)
        {
            // handle conflicts
            if (shortcutWindow.mConflictDetected)
            {
                shortcutWindow.mConflictAction->mKey    = -1;
                shortcutWindow.mConflictAction->mCtrl   = false;
                shortcutWindow.mConflictAction->mAlt    = false;
            }

            // adjust the shortcut action
            action->mKey    = shortcutWindow.mKey;
            action->mAlt    = shortcutWindow.mAlt;
            action->mCtrl   = shortcutWindow.mCtrl;

            // save the new shortcuts
            QSettings settings(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioKeyboardShortcuts.cfg").c_str(), QSettings::IniFormat, this);
            shortcutManager->Save(&settings);

            // reinit the window
            ReInit();
        }
        mShortcutReceiverDialog = nullptr;
    }


    // construct a text version of a shortcut
    QString KeyboardShortcutsWindow::ConstructStringFromShortcut(int key, bool ctrl, bool alt)
    {
        if (key == -1)
        {
            return "not set";
        }

        QString keyText;

        if (ctrl)
        {
            #if AZ_TRAIT_OS_PLATFORM_APPLE
                keyText += "COMMAND + ";
            #else
                keyText += "CTRL + ";
            #endif  
        }
        if (alt)
        {
            keyText += "ALT + ";
        }

        keyText += QKeySequence(key).toString(QKeySequence::NativeText);

        return keyText;
    }


    // reset to default after pressing context menu
    void KeyboardShortcutsWindow::OnResetToDefault()
    {
        if (mContextMenuAction == nullptr)
        {
            return;
        }

        mContextMenuAction->mKey    = mContextMenuAction->mDefaultKey;
        mContextMenuAction->mCtrl   = mContextMenuAction->mDefaultCtrl;
        mContextMenuAction->mAlt    = mContextMenuAction->mDefaultAlt;

        ReInit();
    }


    // assign a new key after pressing the context menu item
    void KeyboardShortcutsWindow::OnAssignNewKey()
    {
        if (mContextMenuAction == nullptr)
        {
            return;
        }

        // assign the new shortcut
        OnShortcutChange(mContextMenuActionIndex, 0);
    }


    // context menu when right clicking on one of the shortcuts
    void KeyboardShortcutsWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // find the table widget item at the clicked position
        QTableWidgetItem* clickedItem = mTableWidget->itemAt(mTableWidget->viewport()->mapFromGlobal(event->globalPos()));
        if (clickedItem == nullptr)
        {
            return;
        }

        int actionIndex = clickedItem->row();

        // get access to the group
        MysticQt::KeyboardShortcutManager::Group* group = GetCurrentGroup();

        // get access to the action
        mContextMenuAction      = group->GetAction(actionIndex);
        mContextMenuActionIndex = actionIndex;

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

        // find the initial shortcut
        //MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        mOrgAction          = action;
        mOrgGroup           = group;

        mConflictAction     = nullptr;
        mConflictDetected   = false;
        mKey                = action->mKey;
        mCtrl               = action->mCtrl;
        mAlt                = action->mAlt;

        QString keyText = KeyboardShortcutsWindow::ConstructStringFromShortcut(mKey, mCtrl, mAlt);

        mLabel = new QLabel(keyText);
        mLabel->setAlignment(Qt::AlignHCenter);
        QFont font = mLabel->font();
        font.setPointSize(14);
        font.setBold(true);
        mLabel->setFont(font);
        layout->addWidget(mLabel);

        mConflictKeyLabel = new QLabel("");
        mConflictKeyLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(mConflictKeyLabel);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setMargin(0);

        mOKButton = new QPushButton("OK");
        buttonLayout->addWidget(mOKButton);
        connect(mOKButton, &QPushButton::clicked, this, &ShortcutReceiverDialog::accept);

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
        mKey    = mOrgAction->mDefaultKey;
        mCtrl   = mOrgAction->mDefaultCtrl;
        mAlt    = mOrgAction->mDefaultAlt;

        UpdateInterface();
    }


    // update the shortcut text and other interface elements
    void ShortcutReceiverDialog::UpdateInterface()
    {
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        QString keyText = KeyboardShortcutsWindow::ConstructStringFromShortcut(mKey, mCtrl, mAlt);

        // check if the currently assigned shortcut is already taken by another shortcut
        mConflictAction = shortcutManager->FindShortcut(mKey, mCtrl, mAlt, mOrgGroup);
        if (mConflictAction == nullptr || mConflictAction == mOrgAction)
        {
            mOKButton->setToolTip("");
            mLabel->setStyleSheet("");
            mConflictKeyLabel->setStyleSheet("");
            mConflictKeyLabel->setText("");
            mConflictDetected = false;
        }
        else
        {
            mLabel->setStyleSheet("color: rgb(244, 156, 28);");
            mConflictKeyLabel->setStyleSheet("color: rgb(244, 156, 28);");

            mConflictDetected = true;

            if (mConflictAction)
            {
                AZStd::string tempString;

                tempString = AZStd::string::format("Assigning new shortcut will unassign '%s' automatically.", mConflictAction->mName.c_str());
                mOKButton->setToolTip(tempString.c_str());

                MysticQt::KeyboardShortcutManager::Group* conflictGroup = shortcutManager->FindGroupForShortcut(mConflictAction);
                if (conflictGroup)
                {
                    tempString = AZStd::string::format("Conflicts with: %s -> %s", conflictGroup->GetName(), mConflictAction->mName.c_str());
                }
                else
                {
                    tempString = AZStd::string::format("Conflicts with: %s", mConflictAction->mName.c_str());
                }

                mConflictKeyLabel->setText(tempString.c_str());
            }
        }

        // adjust the label text to the new shortcut
        mLabel->setText(keyText);
    }


    // close dialog as soon as it lost focus
    void ShortcutReceiverDialog::focusOutEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        //  if (event->reason() == Qt::ActiveWindowFocusReason)
        //      reject();
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
            //mKey      = mOrgAction->mKey;
            //mCtrl     = mOrgAction->mCtrl;
            //mAlt      = mOrgAction->mAlt;

            // close the dialog when pressing ESC
            reject();
        }
        else
        {
            mKey    = event->key();
            mCtrl   = event->modifiers() & Qt::ControlModifier;
            mAlt    = event->modifiers() & Qt::AltModifier;
        }

        UpdateInterface();

        event->accept();
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_KeyboardShortcutsWindow.cpp>
