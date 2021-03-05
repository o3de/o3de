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
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "../StandardPluginsConfig.h"
#include "ParameterWidget.h"
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
     * connect( mParameterSelectionWindow,                                              SIGNAL(rejected()),             this, SLOT(UserWantsToCancel_1()) );
     * connect( mParameterSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),   SIGNAL(itemSelectionChanged()), this, SLOT(SelectionChanged_2()) );
     * connect( mParameterSelectionWindow->GetNodeHierarchyWidget(),                    SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(FinishedSelectionAndPressedOK_3(MCore::Array<SelectionItem>)) );
    */
    class ParameterSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        ParameterSelectionWindow(QWidget* parent, bool useSingleSelection);
        virtual ~ParameterSelectionWindow();

        MCORE_INLINE ParameterWidget* GetParameterWidget()                                                      { return mParameterWidget; }
        void Update(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& selectedParameters)    { mParameterWidget->Update(animGraph, selectedParameters); }

    public slots:
        void OnAccept();
        void OnDoubleClicked(const AZStd::string& item);

    private:
        ParameterWidget*                    mParameterWidget;
        QPushButton*                        mOKButton;
        QPushButton*                        mCancelButton;
        bool                                mUseSingleSelection;
        bool                                mAccepted;
    };
} // namespace EMStudio