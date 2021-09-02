/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendNodeSelectionWindow.h>
#include <QPushButton>
#include <QVBoxLayout>


namespace EMStudio
{
    // constructor
    BlendNodeSelectionWindow::BlendNodeSelectionWindow(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Blend Node Selection Window");

        QVBoxLayout* layout = new QVBoxLayout();

        m_hierarchyWidget = new AnimGraphHierarchyWidget(this);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        m_okButton       = new QPushButton("OK");
        m_cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        layout->addWidget(m_hierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        setMinimumSize(QSize(400, 400));

        m_okButton->setEnabled(false);

        connect(m_okButton, &QPushButton::clicked, this, &BlendNodeSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &BlendNodeSelectionWindow::reject);
        connect(m_hierarchyWidget, &AnimGraphHierarchyWidget::OnSelectionDone, this, &BlendNodeSelectionWindow::OnNodeSelected);
        connect(m_hierarchyWidget, &AnimGraphHierarchyWidget::OnSelectionChanged, this, &BlendNodeSelectionWindow::OnSelectionChanged);
    }


    // destructor
    BlendNodeSelectionWindow::~BlendNodeSelectionWindow()
    {
    }


    void BlendNodeSelectionWindow::OnNodeSelected()
    {
        if (m_useSingleSelection)
        {
            accept();
        }
    }


    void BlendNodeSelectionWindow::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(selected);
        AZ_UNUSED(deselected);

        m_okButton->setEnabled(m_hierarchyWidget->HasSelectedItems());
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_BlendNodeSelectionWindow.cpp>
