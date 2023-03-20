/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_accepted = false;
        setWindowTitle("Parameter Selection Window");

        QVBoxLayout* layout = new QVBoxLayout();

        m_parameterWidget = new ParameterWidget(this, useSingleSelection);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        m_okButton       = new QPushButton("OK");
        m_cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        layout->addWidget(m_parameterWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(m_okButton, &QPushButton::clicked, this, &ParameterSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &ParameterSelectionWindow::reject);
        connect(this, &ParameterSelectionWindow::accepted, this, &ParameterSelectionWindow::OnAccept);
        connect(m_parameterWidget, &ParameterWidget::OnDoubleClicked, this, &ParameterSelectionWindow::OnDoubleClicked);

        // set the selection mode
        m_parameterWidget->SetSelectionMode(useSingleSelection);
        m_useSingleSelection = useSingleSelection;
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
        m_parameterWidget->FireSelectionDoneSignal();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ParameterSelectionWindow.cpp>
