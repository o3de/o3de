/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        void UpdateInterface();

        QKeySequence                                mKey;
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

        static QString ConstructStringFromShortcut(QKeySequence key);
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
    };
} // namespace EMStudio
