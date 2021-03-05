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
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "EMStudioConfig.h"
#include "MotionSetHierarchyWidget.h"
#include <QDialog>
#endif


namespace EMStudio
{
    class EMSTUDIO_API MotionSetSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionSetSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MotionSetSelectionWindow(QWidget* parent, bool useSingleSelection = true, CommandSystem::SelectionList* selectionList = nullptr);
        virtual ~MotionSetSelectionWindow();

        MCORE_INLINE MotionSetHierarchyWidget* GetHierarchyWidget()                                             { return mHierarchyWidget; }
        void Update(EMotionFX::MotionSet* motionSet, CommandSystem::SelectionList* selectionList = nullptr)     { mHierarchyWidget->Update(motionSet, selectionList); }

        void Select(const AZStd::vector<MotionSetSelectionItem>& selectedItems);
        void Select(const AZStd::vector<AZStd::string>& selectedMotionIds, EMotionFX::MotionSet* motionSet);

    public slots:
        void OnAccept();
        void OnSelectionChanged(AZStd::vector<MotionSetSelectionItem> selection);

    private:
        MotionSetHierarchyWidget*   mHierarchyWidget;
        QPushButton*                mOKButton;
        QPushButton*                mCancelButton;
        bool                        mUseSingleSelection;
    };
} // namespace EMStudio