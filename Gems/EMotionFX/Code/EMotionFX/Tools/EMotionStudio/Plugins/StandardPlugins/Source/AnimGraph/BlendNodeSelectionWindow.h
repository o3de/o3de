/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <MCore/Source/StandardHeaders.h>
#include <QDialog>
#endif

struct AnimGraphSelectionItem;

namespace EMStudio
{
    class AnimGraphHierarchyWidget;
    class AnimGraphPlugin;

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
    class BlendNodeSelectionWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(BlendNodeSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        BlendNodeSelectionWindow(QWidget* parent = nullptr);
        virtual ~BlendNodeSelectionWindow();

        AnimGraphHierarchyWidget& GetAnimGraphHierarchyWidget() { return *mHierarchyWidget; }

    public slots:
        void OnNodeSelected();
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        AnimGraphHierarchyWidget*           mHierarchyWidget;
        QPushButton*                        mOKButton;
        QPushButton*                        mCancelButton;
        bool                                mUseSingleSelection;
    };
} // namespace EMStudio
