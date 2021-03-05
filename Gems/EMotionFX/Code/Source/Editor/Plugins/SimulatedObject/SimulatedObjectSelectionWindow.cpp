/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        m_OKButton = new QPushButton("OK");
        m_cancelButton = new QPushButton("Cancel");

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(m_OKButton);
        buttonLayout->addWidget(m_cancelButton);

        QVBoxLayout* layout = new QVBoxLayout(this);
        m_simulatedObjectSelectionWidget = new SimulatedObjectSelectionWidget(this);
        layout->addWidget(m_simulatedObjectSelectionWidget);
        layout->addLayout(buttonLayout);

        connect(m_OKButton, &QPushButton::clicked, this, &SimulatedObjectSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &SimulatedObjectSelectionWindow::reject);
        connect(m_simulatedObjectSelectionWidget, &SimulatedObjectSelectionWidget::OnDoubleClicked, this, &SimulatedObjectSelectionWindow::accept);
    }
} // namespace EMStudio
