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

#include <AzQtComponents/Application/Window/AzQtApplicationWindow.h>
#include <QVBoxLayout>


namespace AzQtComponents
{
    AzQtApplicationWindow::AzQtApplicationWindow(QWidget* parent /* = 0 */, const AZStd::string& objectName)
        : AzQtComponents::DockMainWindow(parent)
    {
        m_advancedDockManager = new AzQtComponents::FancyDocking(this);

        setObjectName(objectName.c_str());
        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        m_menuBar = new QMenuBar(this);
        m_menuBar->setObjectName("MenuBar");
        setMenuBar(m_menuBar);

        m_centralWidget = new QWidget(this);
        m_tabWidget = new AzQtComponents::TabWidget(m_centralWidget);
        m_tabWidget->setObjectName("TabWidget");
        m_tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_tabWidget->setContentsMargins(0, 0, 0, 0);

        vl = new QVBoxLayout(m_centralWidget);
    }

    void AzQtApplicationWindow::SelectPreviousTab()
    {
        if (m_tabWidget->count() > 1)
        {
            // Adding count to wrap around when index <= 0
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + m_tabWidget->count() - 1) % m_tabWidget->count());
        }
    }

    void AzQtApplicationWindow::SelectNextTab()
    {
        if (m_tabWidget->count() > 1)
        {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % m_tabWidget->count());
        }
    }
}
    
