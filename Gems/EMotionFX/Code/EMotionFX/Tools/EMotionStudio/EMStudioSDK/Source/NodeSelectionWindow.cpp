/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_accepted = false;
        setWindowTitle("Node Selection Window");

        QVBoxLayout* layout = new QVBoxLayout();

        m_hierarchyWidget = new NodeHierarchyWidget(this, useSingleSelection);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        m_okButton       = new QPushButton("OK");
        m_cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        layout->addWidget(m_hierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(m_okButton, &QPushButton::clicked, this, &NodeSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &NodeSelectionWindow::reject);
        connect(this, &NodeSelectionWindow::accepted, this, &NodeSelectionWindow::OnAccept);
        connect(m_hierarchyWidget, static_cast<void (NodeHierarchyWidget::*)(AZStd::vector<SelectionItem>)>(&NodeHierarchyWidget::OnDoubleClicked), this, &NodeSelectionWindow::OnDoubleClicked);

        // connect the window activation signal to refresh if reactivated
        //connect( this, SIGNAL(visibilityChanged(bool)), this, SLOT(OnVisibilityChanged(bool)) );

        // set the selection mode
        m_hierarchyWidget->SetSelectionMode(useSingleSelection);
        m_useSingleSelection = useSingleSelection;

        setMinimumSize(QSize(500, 400));
        resize(700, 800);
    }


    void NodeSelectionWindow::OnDoubleClicked(AZStd::vector<SelectionItem> selection)
    {
        MCORE_UNUSED(selection);
        accept();
    }


    void NodeSelectionWindow::OnAccept()
    {
        m_hierarchyWidget->FireSelectionDoneSignal();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_NodeSelectionWindow.cpp>
