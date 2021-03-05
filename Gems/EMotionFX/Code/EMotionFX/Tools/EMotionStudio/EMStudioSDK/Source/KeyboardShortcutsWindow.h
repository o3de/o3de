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
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/MemoryCategories.h>
#include "EMStudioConfig.h"
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#endif



namespace EMStudio
{
    class EMSTUDIO_API ShortcutReceiverDialog
        : public QDialog
    {
        Q_OBJECT
                       MCORE_MEMORYOBJECTCATEGORY(ShortcutReceiverDialog, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        ShortcutReceiverDialog(QWidget* parent, MysticQt::KeyboardShortcutManager::Action* action, MysticQt::KeyboardShortcutManager::Group* group);
        virtual ~ShortcutReceiverDialog() {}

        void keyPressEvent(QKeyEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void UpdateInterface();

        int                                         mKey;
        bool                                        mCtrl;
        bool                                        mAlt;
        bool                                        mConflictDetected;
        MysticQt::KeyboardShortcutManager::Action*  mConflictAction;

    private slots:
        void ResetToDefault();
    private:
        QLabel*                                     mLabel;
        QLabel*                                     mConflictKeyLabel;
        QPushButton*                                mOKButton;
        MysticQt::KeyboardShortcutManager::Action*  mOrgAction;
        MysticQt::KeyboardShortcutManager::Group*   mOrgGroup;
    };


    class KeyboardShortcutsWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(KeyboardShortcutsWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK);

    public:
        KeyboardShortcutsWindow(QWidget* parent);
        ~KeyboardShortcutsWindow();

        void Init();
        void ReInit();

        static QString ConstructStringFromShortcut(int key, bool ctrl, bool alt);
        MysticQt::KeyboardShortcutManager::Group* GetCurrentGroup() const;

        void setVisible(bool visible) override;

    private slots:
        void OnGroupSelectionChanged();
        void OnShortcutChange(int row, int column);
        void OnResetToDefault();
        void OnAssignNewKey();

    private:
        QTableWidget*                               mTableWidget;
        QListWidget*                                mListWidget;
        QHBoxLayout*                                mHLayout;
        int                                         mSelectedGroup;
        MysticQt::KeyboardShortcutManager::Action*  mContextMenuAction;
        int                                         mContextMenuActionIndex;
        ShortcutReceiverDialog*                     mShortcutReceiverDialog;

        void contextMenuEvent(QContextMenuEvent* event) override;
        void hideEvent(QHideEvent* event) override;
    };
} // namespace EMStudio
