/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Window/AtomToolsMainWindow.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    AtomToolsMainWindow::AtomToolsMainWindow(QWidget* parent)
        : AzQtComponents::DockMainWindow(parent)
    {
        m_advancedDockManager = new AzQtComponents::FancyDocking(this);

        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        AddCommonMenus();

        m_statusMessage = new QLabel(statusBar());
        statusBar()->addPermanentWidget(m_statusMessage, 1);

        auto centralWidget = new QWidget(this);
        auto centralWidgetLayout = new QVBoxLayout(centralWidget);
        centralWidgetLayout->setMargin(0);
        centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
        centralWidget->setLayout(centralWidgetLayout);
        setCentralWidget(centralWidget);

        AtomToolsMainWindowRequestBus::Handler::BusConnect();
    }

    AtomToolsMainWindow::~AtomToolsMainWindow()
    {
        AtomToolsMainWindowRequestBus::Handler::BusDisconnect();
    }

    void AtomToolsMainWindow::ActivateWindow()
    {
        show();
        raise();
        activateWindow();
    }

    bool AtomToolsMainWindow::AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end() || !widget)
        {
            return false;
        }

        auto dockWidget = new AzQtComponents::StyledDockWidget(name.c_str());
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        widget->setObjectName(name.c_str());
        widget->setParent(dockWidget);
        widget->setMinimumSize(QSize(300, 300));
        dockWidget->setWidget(widget);
        addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);
        resizeDocks({ dockWidget }, { 400 }, aznumeric_cast<Qt::Orientation>(orientation));
        m_dockWidgets[name] = dockWidget;

        m_dockActions[name] = m_menuView->addAction(name.c_str(), [this, name](){
            SetDockWidgetVisible(name, !IsDockWidgetVisible(name));
        });
        return true;
    }

    void AtomToolsMainWindow::RemoveDockWidget(const AZStd::string& name)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            delete dockWidgetItr->second;
            m_dockWidgets.erase(dockWidgetItr);
        }
        auto dockActionItr = m_dockActions.find(name);
        if (dockActionItr != m_dockActions.end())
        {
            delete dockActionItr->second;
            m_dockActions.erase(dockActionItr);
        }
    }

    void AtomToolsMainWindow::SetDockWidgetVisible(const AZStd::string& name, bool visible)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            dockWidgetItr->second->setVisible(visible);
        }
    }

    bool AtomToolsMainWindow::IsDockWidgetVisible(const AZStd::string& name) const
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            return dockWidgetItr->second->isVisible();
        }
        return false;
    }

    AZStd::vector<AZStd::string> AtomToolsMainWindow::GetDockWidgetNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(m_dockWidgets.size());
        for (const auto& dockWidgetPair : m_dockWidgets)
        {
            names.push_back(dockWidgetPair.first);
        }
        return names;
    }

    void AtomToolsMainWindow::SetStatusMessage(const QString& message)
    {
        m_statusMessage->setText(QString("<font color=\"White\">%1</font>").arg(message));
    }

    void AtomToolsMainWindow::SetStatusWarning(const QString& message)
    {
        m_statusMessage->setText(QString("<font color=\"Yellow\">%1</font>").arg(message));
    }

    void AtomToolsMainWindow::SetStatusError(const QString& message)
    {
        m_statusMessage->setText(QString("<font color=\"Red\">%1</font>").arg(message));
    }

    void AtomToolsMainWindow::AddCommonMenus()
    {
        m_menuFile = menuBar()->addMenu("&File");
        m_menuEdit = menuBar()->addMenu("&Edit");
        m_menuView = menuBar()->addMenu("&View");
        m_menuHelp = menuBar()->addMenu("&Help");

        m_menuFile->addAction("Run &Python...", [this]() {
            const QString script = QFileDialog::getOpenFileName(this, "Run Script", QString(), QString("*.py"));
            if (!script.isEmpty())
            {
                AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, script.toUtf8().constData());
            }
        });

        m_menuFile->addSeparator();

        m_menuFile->addAction("E&xit", [this]() {
            close();
        }, QKeySequence::Quit);

        m_menuEdit->addAction("&Settings...", [this]() {
            OpenSettings();
        }, QKeySequence::Preferences);

        m_menuHelp->addAction("&Help...", [this]() {
            OpenHelp();
        });

        m_menuHelp->addAction("&About...", [this]() {
            OpenAbout();
        });
    }

    void AtomToolsMainWindow::OpenSettings()
    {
    }

    void AtomToolsMainWindow::OpenHelp()
    {
    }

    void AtomToolsMainWindow::OpenAbout()
    {
    }
} // namespace AtomToolsFramework
