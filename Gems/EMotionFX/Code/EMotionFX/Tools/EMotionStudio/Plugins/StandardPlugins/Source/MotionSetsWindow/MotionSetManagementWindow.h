/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QAction)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declaration
    class MotionSetsWindowPlugin;


    class MotionSetManagementRemoveMotionsFailedWindow
        : public QDialog
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionSetManagementRemoveMotionsFailedWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionSetManagementRemoveMotionsFailedWindow(QWidget* parent, const MCore::Array<EMotionFX::Motion*>& motions);
    };


    class MotionSetManagementRenameWindow
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionSetManagementRenameWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionSetManagementRenameWindow(QWidget* parent, EMotionFX::MotionSet* motionSet);

    private slots:
        void TextEdited(const QString& text);
        void Accepted();

    private:
        EMotionFX::MotionSet*   mMotionSet;
        QLineEdit*              mLineEdit;
        QPushButton*            mOKButton;
        //QLabel*               mErrorMsg;
    };


    class MotionSetManagementWindow
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionSetManagementWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionSetManagementWindow(MotionSetsWindowPlugin* parentPlugin, QWidget* parent);
        ~MotionSetManagementWindow();

        bool Init();
        void ReInit();

        void SelectItemsById(uint32 motionSetId);
        void GetSelectedMotionSets(AZStd::vector<EMotionFX::MotionSet*>& outSelectedMotionSets) const;

        static void RecursiveRemoveMotionsFromSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup& commandGroup, AZStd::vector<EMotionFX::Motion*>& failedRemoveMotions);

    public slots:
        void UpdateInterface();

        void OnCreateMotionSet();
        void OnRemoveSelectedMotionSets();
        void OnRenameSelectedMotionSet();
        void OnClearMotionSets();

        void OnSelectionChanged();

        //void OnAdjustMotionSetDirectory();

        void OnOpen();
        void OnSave();
        void OnSaveAs();

        void OnTextFilterChanged(const QString& text);

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void RecursiveIncreaseMotionsReferenceCount(EMotionFX::MotionSet* motionSet);
        void RecursivelyAddSets(QTreeWidgetItem* parent, EMotionFX::MotionSet* motionSet, const AZStd::vector<uint32>& selectedSetIDs);
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        QVBoxLayout* mVLayout = nullptr;
        QTreeWidget* mMotionSetsTree = nullptr;
        QAction* m_addAction = nullptr;
        QAction* m_openAction = nullptr;
        QAction* m_saveMenuAction = nullptr;
        QAction* m_saveAction = nullptr;
        QAction* m_saveAsAction = nullptr;
        AzQtComponents::FilteredSearchWidget* m_searchWidget = nullptr;
        AZStd::string m_searchWidgetText;
        MotionSetsWindowPlugin* mPlugin = nullptr;
    };
} // namespace EMStudio
