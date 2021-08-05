/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QMenu>
#include <QAction>

#include <AzQtComponents/Components/Widgets/SegmentBar.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingWindow.h>
#include <Editor/View/Widgets/LoggingPanel/ui_LoggingWindow.h>

namespace ScriptCanvasEditor
{
    //////////////////
    // LoggingWindow
    //////////////////

    LoggingWindow::LoggingWindow(QWidget* parentWidget)
        : AzQtComponents::StyledDockWidget(parentWidget)
        , m_ui(new Ui::LoggingWindow)
    {
        m_ui->setupUi(this);

        // Hack to hide the close button on the first tab. Since we always want it open.
        m_ui->tabWidget->setTabsClosable(true);
        m_ui->tabWidget->tabBar()->setTabButton(0, QTabBar::ButtonPosition::RightSide, nullptr);
        m_ui->tabWidget->tabBar()->setTabButton(0, QTabBar::ButtonPosition::LeftSide, nullptr);

        m_ui->segmentWidget->addTab(new QWidget(m_ui->segmentWidget), QStringLiteral("Entities"));
        m_ui->segmentWidget->addTab(new QWidget(m_ui->segmentWidget), QStringLiteral("Graphs"));

        connect(m_ui->segmentWidget, &AzQtComponents::SegmentControl::currentChanged, [this](int newIndex) {
            m_ui->stackedWidget->setCurrentIndex(newIndex);
        });

        QObject::connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &LoggingWindow::OnActiveTabChanged);

        AzQtComponents::TabWidget::applySecondaryStyle(m_ui->tabWidget, false);

        m_entityPageIndex = m_ui->stackedWidget->indexOf(m_ui->entitiesPage);
        m_graphPageIndex = m_ui->stackedWidget->indexOf(m_ui->graphsPage);

        OnActiveTabChanged(m_ui->tabWidget->currentIndex());
        PivotOnEntities();
    }

    LoggingWindow::~LoggingWindow()
    {

    }

    void LoggingWindow::OnActiveTabChanged([[maybe_unused]] int index)
    {
        LoggingWindowSession* windowSession = qobject_cast<LoggingWindowSession*>(m_ui->tabWidget->currentWidget());

        if (windowSession)
        {
            m_activeDataId = windowSession->GetDataId();
        }

        m_ui->entityPivotWidget->SwitchDataSource(m_activeDataId);
        m_ui->graphPivotWidget->SwitchDataSource(m_activeDataId);
    }

    void LoggingWindow::PivotOnEntities()
    {
        m_ui->stackedWidget->setCurrentIndex(m_ui->stackedWidget->indexOf(m_ui->entitiesPage));
    }

    void LoggingWindow::PivotOnGraphs()
    {
        m_ui->stackedWidget->setCurrentIndex(m_ui->stackedWidget->indexOf(m_ui->graphsPage));
    }

    PivotTreeWidget* LoggingWindow::GetActivePivotWidget() const
    {
        if (m_ui->stackedWidget->currentIndex() == m_entityPageIndex)
        {
            return m_ui->entityPivotWidget;
        }

        return nullptr;
    }

#include <Editor/View/Widgets/LoggingPanel/moc_LoggingWindow.cpp>
}
