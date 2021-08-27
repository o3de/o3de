/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/SimulatedObject/SimulatedObjectSelectionWindow.h>
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
    SimulatedObjectSelectionWindow::SimulatedObjectSelectionWindow(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("SimulatedObject Selection Window");

        m_okButton = new QPushButton("OK");
        m_cancelButton = new QPushButton("Cancel");

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        QVBoxLayout* layout = new QVBoxLayout(this);
        m_simulatedObjectSelectionWidget = new SimulatedObjectSelectionWidget(this);
        layout->addWidget(m_simulatedObjectSelectionWidget);
        layout->addLayout(buttonLayout);

        connect(m_okButton, &QPushButton::clicked, this, &SimulatedObjectSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &SimulatedObjectSelectionWindow::reject);
        connect(m_simulatedObjectSelectionWidget, &SimulatedObjectSelectionWidget::OnDoubleClicked, this, &SimulatedObjectSelectionWindow::accept);
    }
} // namespace EMStudio
