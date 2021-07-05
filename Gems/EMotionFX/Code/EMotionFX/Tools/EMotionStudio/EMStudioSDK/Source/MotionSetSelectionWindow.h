/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
