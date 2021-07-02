/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "NodeSelectionWindow.h"
#include "EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Mesh.h>
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QGraphicsDropShadowEffect>


namespace EMStudio
{
    // constructor
    NodeSelectionWindow::NodeSelectionWindow(QWidget* parent, bool useSingleSelection)
        : QDialog(parent)
    {
        mAccepted = false;
        setWindowTitle("Node Selection Window");

        QVBoxLayout* layout = new QVBoxLayout();

        mHierarchyWidget = new NodeHierarchyWidget(this, useSingleSelection);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mHierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, &QPushButton::clicked, this, &NodeSelectionWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &NodeSelectionWindow::reject);
        connect(this, &NodeSelectionWindow::accepted, this, &NodeSelectionWindow::OnAccept);
        connect(mHierarchyWidget, static_cast<void (NodeHierarchyWidget::*)(MCore::Array<SelectionItem>)>(&NodeHierarchyWidget::OnDoubleClicked), this, &NodeSelectionWindow::OnDoubleClicked);

        // connect the window activation signal to refresh if reactivated
        //connect( this, SIGNAL(visibilityChanged(bool)), this, SLOT(OnVisibilityChanged(bool)) );

        // set the selection mode
        mHierarchyWidget->SetSelectionMode(useSingleSelection);
        mUseSingleSelection = useSingleSelection;

        setMinimumSize(QSize(500, 400));
        resize(700, 800);
    }


    void NodeSelectionWindow::OnDoubleClicked(MCore::Array<SelectionItem> selection)
    {
        MCORE_UNUSED(selection);
        accept();
    }


    void NodeSelectionWindow::OnAccept()
    {
        mHierarchyWidget->FireSelectionDoneSignal();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_NodeSelectionWindow.cpp>
