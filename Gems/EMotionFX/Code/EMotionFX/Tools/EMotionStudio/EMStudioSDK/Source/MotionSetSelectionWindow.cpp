/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionSetSelectionWindow.h"
#include "EMStudioManager.h"
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>


namespace EMStudio
{
    MotionSetSelectionWindow::MotionSetSelectionWindow(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList)
        : QDialog(parent)
    {
        setWindowTitle("Motion Selection Window");
        resize(850, 500);

        QVBoxLayout* layout = new QVBoxLayout();

        mHierarchyWidget = new MotionSetHierarchyWidget(this, useSingleSelection, selectionList);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mOKButton->setObjectName("EMFX.MotionSetSelectionWindow.Ok");
        mCancelButton   = new QPushButton("Cancel");
        mCancelButton->setObjectName("EMFX.MotionSetSelectionWindow.Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mHierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, &QPushButton::clicked, this, &MotionSetSelectionWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &MotionSetSelectionWindow::reject);
        connect(this, &MotionSetSelectionWindow::accepted, this, &MotionSetSelectionWindow::OnAccept);
        connect(mHierarchyWidget, &MotionSetHierarchyWidget::SelectionChanged, this, &MotionSetSelectionWindow::OnSelectionChanged);

        // set the selection mode
        mHierarchyWidget->SetSelectionMode(useSingleSelection);
        mUseSingleSelection = useSingleSelection;
    }


    MotionSetSelectionWindow::~MotionSetSelectionWindow()
    {
    }


    void MotionSetSelectionWindow::Select(const AZStd::vector<MotionSetSelectionItem>& selectedItems)
    {
        mHierarchyWidget->Select(selectedItems);
    }


    void MotionSetSelectionWindow::Select(const AZStd::vector<AZStd::string>& selectedMotionIds, EMotionFX::MotionSet* motionSet)
    {
        AZStd::vector<MotionSetSelectionItem> selectedItems;

        for (const AZStd::string& motionId : selectedMotionIds)
        {
            selectedItems.push_back(MotionSetSelectionItem(motionId, motionSet));
        }

        Select(selectedItems);
    }


    void MotionSetSelectionWindow::OnSelectionChanged(AZStd::vector<MotionSetSelectionItem> selection)
    {
        MCORE_UNUSED(selection);
        accept();
    }


    void MotionSetSelectionWindow::OnAccept()
    {
        if (mUseSingleSelection == false)
        {
            mHierarchyWidget->FireSelectionDoneSignal();
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_MotionSetSelectionWindow.cpp>
