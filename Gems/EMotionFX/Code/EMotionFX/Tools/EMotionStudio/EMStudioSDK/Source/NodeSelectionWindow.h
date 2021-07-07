/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_NODESELECTIONWINDOW_H
#define __EMSTUDIO_NODESELECTIONWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "EMStudioConfig.h"
#include "NodeHierarchyWidget.h"
#include <QDialog>
#endif



namespace EMStudio
{
    /*
     * How to use this dialog?
     * 1. Use the rejected() signal to catch when the X on the window or the cancel button is pressed.
     * 2. Use the itemSelectionChanged() signal of the GetNodeHierarchyWidget()->GetTreeWidget() to detect when the user adjusts the selection in the node hierarchy widget.
     * 3. Use the OnSelectionDone() in the GetNodeHierarchyWidget() to detect when the user finished selecting and pressed the OK button.
     * Example:
     * connect( mNodeSelectionWindow,                                               SIGNAL(rejected()),             this, SLOT(UserWantsToCancel_1()) );
     * connect( mNodeSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),    SIGNAL(itemSelectionChanged()), this, SLOT(SelectionChanged_2()) );
     * connect( mNodeSelectionWindow->GetNodeHierarchyWidget(),                     SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(FinishedSelectionAndPressedOK_3(MCore::Array<SelectionItem>)) );
    */
    class EMSTUDIO_API NodeSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        NodeSelectionWindow(QWidget* parent, bool useSingleSelection);

        MCORE_INLINE NodeHierarchyWidget* GetNodeHierarchyWidget()                                                                { return mHierarchyWidget; }
        void Update(uint32 actorInstanceID, CommandSystem::SelectionList* selectionList = nullptr)                                { mHierarchyWidget->Update(actorInstanceID, selectionList); }
        void Update(const MCore::Array<uint32>& actorInstanceIDs, CommandSystem::SelectionList* selectionList = nullptr)          { mHierarchyWidget->Update(actorInstanceIDs, selectionList); }

    public slots:
        void OnAccept();
        void OnDoubleClicked(MCore::Array<SelectionItem> selection);

    private:
        NodeHierarchyWidget*                mHierarchyWidget;
        QPushButton*                        mOKButton;
        QPushButton*                        mCancelButton;
        bool                                mUseSingleSelection;
        bool                                mAccepted;
    };
} // namespace EMStudio

#endif
