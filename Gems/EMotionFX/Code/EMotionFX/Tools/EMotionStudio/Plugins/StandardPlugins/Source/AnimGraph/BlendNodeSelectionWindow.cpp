/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        mHierarchyWidget = new AnimGraphHierarchyWidget(this);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mHierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        setMinimumSize(QSize(400, 400));

        mOKButton->setEnabled(false);

        connect(mOKButton, &QPushButton::clicked, this, &BlendNodeSelectionWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &BlendNodeSelectionWindow::reject);
        connect(mHierarchyWidget, &AnimGraphHierarchyWidget::OnSelectionDone, this, &BlendNodeSelectionWindow::OnNodeSelected);
        connect(mHierarchyWidget, &AnimGraphHierarchyWidget::OnSelectionChanged, this, &BlendNodeSelectionWindow::OnSelectionChanged);
    }


    // destructor
    BlendNodeSelectionWindow::~BlendNodeSelectionWindow()
    {
    }


    void BlendNodeSelectionWindow::OnNodeSelected()
    {
        if (mUseSingleSelection)
        {
            accept();
        }
    }


    void BlendNodeSelectionWindow::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(selected);
        AZ_UNUSED(deselected);

        mOKButton->setEnabled(mHierarchyWidget->HasSelectedItems());
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_BlendNodeSelectionWindow.cpp>
