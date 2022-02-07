/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
     * connect( m_nodeSelectionWindow,                                               SIGNAL(rejected()),             this, SLOT(UserWantsToCancel_1()) );
     * connect( m_nodeSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),    SIGNAL(itemSelectionChanged()), this, SLOT(SelectionChanged_2()) );
     * connect( m_nodeSelectionWindow->GetNodeHierarchyWidget(),                     SIGNAL(OnSelectionDone(AZStd::vector<SelectionItem>)), this, SLOT(FinishedSelectionAndPressedOK_3(AZStd::vector<SelectionItem>)) );
    */
    class EMSTUDIO_API NodeSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        NodeSelectionWindow(QWidget* parent, bool useSingleSelection);

        MCORE_INLINE NodeHierarchyWidget* GetNodeHierarchyWidget()                                                                { return m_hierarchyWidget; }
        void Update(uint32 actorInstanceID, CommandSystem::SelectionList* selectionList = nullptr)                                { m_hierarchyWidget->Update(actorInstanceID, selectionList); }
        void Update(const AZStd::vector<uint32>& actorInstanceIDs, CommandSystem::SelectionList* selectionList = nullptr)          { m_hierarchyWidget->Update(actorInstanceIDs, selectionList); }

    public slots:
        void OnAccept();
        void OnDoubleClicked(AZStd::vector<SelectionItem> selection);

    private:
        NodeHierarchyWidget*                m_hierarchyWidget;
        QPushButton*                        m_okButton;
        QPushButton*                        m_cancelButton;
        bool                                m_useSingleSelection;
        bool                                m_accepted;
    };
} // namespace EMStudio

#endif
