/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        m_hierarchyWidget = new MotionSetHierarchyWidget(this, useSingleSelection, selectionList);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        m_okButton       = new QPushButton("OK");
        m_okButton->setObjectName("EMFX.MotionSetSelectionWindow.Ok");
        m_cancelButton   = new QPushButton("Cancel");
        m_cancelButton->setObjectName("EMFX.MotionSetSelectionWindow.Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        layout->addWidget(m_hierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(m_okButton, &QPushButton::clicked, this, &MotionSetSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &MotionSetSelectionWindow::reject);
        connect(this, &MotionSetSelectionWindow::accepted, this, &MotionSetSelectionWindow::OnAccept);
        connect(m_hierarchyWidget, &MotionSetHierarchyWidget::SelectionChanged, this, &MotionSetSelectionWindow::OnSelectionChanged);

        // set the selection mode
        m_hierarchyWidget->SetSelectionMode(useSingleSelection);
        m_useSingleSelection = useSingleSelection;
    }


    MotionSetSelectionWindow::~MotionSetSelectionWindow()
    {
    }


    void MotionSetSelectionWindow::Select(const AZStd::vector<MotionSetSelectionItem>& selectedItems)
    {
        m_hierarchyWidget->Select(selectedItems);
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
        if (m_useSingleSelection == false)
        {
            m_hierarchyWidget->FireSelectionDoneSignal();
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_MotionSetSelectionWindow.cpp>
