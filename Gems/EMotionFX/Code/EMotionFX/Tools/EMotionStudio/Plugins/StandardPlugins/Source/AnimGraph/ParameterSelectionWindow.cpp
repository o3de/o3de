/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParameterSelectionWindow.h"
#include <MCore/Source/LogManager.h>
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
    ParameterSelectionWindow::ParameterSelectionWindow(QWidget* parent, bool useSingleSelection)
        : QDialog(parent)
    {
        mAccepted = false;
        setWindowTitle("Parameter Selection Window");

        QVBoxLayout* layout = new QVBoxLayout();

        mParameterWidget = new ParameterWidget(this, useSingleSelection);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mParameterWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, &QPushButton::clicked, this, &ParameterSelectionWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &ParameterSelectionWindow::reject);
        connect(this, &ParameterSelectionWindow::accepted, this, &ParameterSelectionWindow::OnAccept);
        connect(mParameterWidget, &ParameterWidget::OnDoubleClicked, this, &ParameterSelectionWindow::OnDoubleClicked);

        // set the selection mode
        mParameterWidget->SetSelectionMode(useSingleSelection);
        mUseSingleSelection = useSingleSelection;
    }


    ParameterSelectionWindow::~ParameterSelectionWindow()
    {
    }


    void ParameterSelectionWindow::OnDoubleClicked(const AZStd::string& item)
    {
        MCORE_UNUSED(item);
        accept();
    }


    void ParameterSelectionWindow::OnAccept()
    {
        mParameterWidget->FireSelectionDoneSignal();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ParameterSelectionWindow.cpp>
