/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <AtomToolsFramework/PerformanceMonitor/PerformanceMonitorRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindow.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowNotificationBus.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>

#include <QCloseEvent>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    AtomToolsMainWindow::AtomToolsMainWindow(const AZ::Crc32& toolId, const QString& objectName, QWidget* parent)
        : Base(parent)
        , m_toolId(toolId)
        , m_advancedDockManager(new AzQtComponents::FancyDocking(this, objectName.toUtf8().constData()))
        , m_mainWindowWrapper(new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons))
    {
        setObjectName(objectName);

        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        m_statusMessage = new QLabel(statusBar());
        statusBar()->addPermanentWidget(m_statusMessage, 1);

        auto centralWidget = new QWidget(this);
        auto centralWidgetLayout = new QVBoxLayout(centralWidget);
        centralWidgetLayout->setMargin(0);
        centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
        centralWidget->setLayout(centralWidgetLayout);
        setCentralWidget(centralWidget);

        m_assetBrowser = new AtomToolsAssetBrowser(this);
        AddDockWidget("Asset Browser", m_assetBrowser, Qt::BottomDockWidgetArea);

        AddDockWidget("Python Terminal", new AzToolsFramework::CScriptTermDialog, Qt::BottomDockWidgetArea);
        SetDockWidgetVisible("Python Terminal", false);

        m_logPanel = new AzToolsFramework::LogPanel::TracePrintFLogPanel(this);
        m_logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Log", "", ""));
        AddDockWidget("Logging", m_logPanel, Qt::BottomDockWidgetArea);
        SetDockWidgetVisible("Logging", false);

        SetupMetrics();
        UpdateWindowTitle();

        resize(1280, 1024);

        // Manage saving window geometry, restoring state window is shown for the first time
        m_mainWindowWrapper->setGuest(this);
        m_mainWindowWrapper->enableSaveRestoreGeometry(
            QApplication::organizationName(), QApplication::applicationName(), "mainWindowGeometry");

        AtomToolsMainWindowRequestBus::Handler::BusConnect(m_toolId);
        AtomToolsMainMenuRequestBus::Handler::BusConnect(m_toolId);
        QueueUpdateMenus(true);
    }

    AtomToolsMainWindow::~AtomToolsMainWindow()
    {
        PerformanceMonitorRequestBus::Broadcast(&PerformanceMonitorRequestBus::Handler::SetProfilerEnabled, false);
        AtomToolsMainWindowRequestBus::Handler::BusDisconnect();
        AtomToolsMainMenuRequestBus::Handler::BusDisconnect();
    }

    void AtomToolsMainWindow::ActivateWindow()
    {
        show();
        raise();
        activateWindow();
    }

    bool AtomToolsMainWindow::AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area)
    {
        auto dockWidget = qobject_cast<QDockWidget*>(widget);
        if (!dockWidget)
        {
            // If the widget being added is not already dockable then add a container dock widget for it
            dockWidget = new AzQtComponents::StyledDockWidget(name.c_str(), this);
            dockWidget->setWidget(widget);
            widget->setWindowTitle(name.c_str());
            widget->setObjectName(QString("%1_Widget").arg(name.c_str()));
            widget->setMinimumSize(QSize(300, 300));
            widget->setParent(dockWidget);
            widget->setVisible(true);
        }

        // Rename, resize, and reparent the dock widget for this main window
        dockWidget->setWindowTitle(name.c_str());
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        dockWidget->setMinimumSize(QSize(300, 300));
        dockWidget->setParent(this);
        dockWidget->setVisible(true);

        addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);
        resizeDocks({ dockWidget }, { 400 }, Qt::Horizontal);
        resizeDocks({ dockWidget }, { 400 }, Qt::Vertical);
        QueueUpdateMenus(true);
        return true;
    }

    void AtomToolsMainWindow::RemoveDockWidget(const AZStd::string& name)
    {
        for (auto dockWidget : findChildren<QDockWidget*>())
        {
            if (dockWidget->windowTitle().compare(name.c_str(), Qt::CaseInsensitive) == 0)
            {
                delete dockWidget;
                QueueUpdateMenus(true);
                break;
            }
        }
    }

    void AtomToolsMainWindow::SetDockWidgetVisible(const AZStd::string& name, bool visible)
    {
        for (auto dockWidget : findChildren<QDockWidget*>())
        {
            if (dockWidget->windowTitle().compare(name.c_str(), Qt::CaseInsensitive) == 0)
            {
                if (auto tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(dockWidget))
                {
                    // If the dock widget is tabbed, then set it as the active tab
                    int index = tabWidget->indexOf(dockWidget);
                    if (visible)
                    {
                        tabWidget->setCurrentIndex(index);
                    }
                    tabWidget->setTabVisible(index, visible);
                }
                else
                {
                    // Otherwise just show the widget
                    m_advancedDockManager->restoreDockWidget(dockWidget);
                }

                dockWidget->setVisible(visible);
                break;
            }
        }
    }

    bool AtomToolsMainWindow::IsDockWidgetVisible(const AZStd::string& name) const
    {
        for (auto dockWidget : findChildren<QDockWidget*>())
        {
            if (dockWidget->windowTitle().compare(name.c_str(), Qt::CaseInsensitive) == 0)
            {
                return dockWidget->isVisible();
            }
        }
        return false;
    }

    AZStd::vector<AZStd::string> AtomToolsMainWindow::GetDockWidgetNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(children().size());
        for (auto dockWidget : findChildren<QDockWidget*>())
        {
            names.push_back(dockWidget->windowTitle().toUtf8().constData());
        }
        AZStd::sort(names.begin(), names.end());
        return names;
    }

    void AtomToolsMainWindow::QueueUpdateMenus(bool rebuildMenus)
    {
        m_rebuildMenus = m_rebuildMenus || rebuildMenus;
        if (!m_updateMenus)
        {
            m_updateMenus = true;
            QTimer::singleShot(0, [this]() {
                if (m_rebuildMenus)
                {
                    // Clearing all actions that were added directly to the menu bar
                    menuBar()->clear();

                    // Instead of destroying and recreating the menu bar, destroying the individual child menus to prevent the UI from
                    // popping when the menu bar is recreated
                    auto menus = menuBar()->findChildren<QMenu*>();
                    for (auto menu : menus)
                    {
                        delete menu;
                    }

                    AtomToolsMainMenuRequestBus::Event(m_toolId, &AtomToolsMainMenuRequestBus::Events::CreateMenus, menuBar());
                }
                AtomToolsMainMenuRequestBus::Event(m_toolId, &AtomToolsMainMenuRequestBus::Events::UpdateMenus, menuBar());
                m_updateMenus = false;
                m_rebuildMenus = false;
            });
        }
    }

    void AtomToolsMainWindow::CreateMenus(QMenuBar* menuBar)
    {
        m_menuFile = menuBar->addMenu("&File");
        m_menuFile->setObjectName("menuFile");
        m_menuEdit = menuBar->addMenu("&Edit");
        m_menuEdit->setObjectName("menuEdit");
        m_menuView = menuBar->addMenu("&View");
        m_menuView->setObjectName("menuView");
        m_menuTools = menuBar->addMenu("&Tools");
        m_menuTools->setObjectName("menuTools");
        m_menuHelp = menuBar->addMenu("&Help");
        m_menuHelp->setObjectName("menuHelp");

        m_menuFile->addAction("Run &Python...", [this]() {
            const QString script = QFileDialog::getOpenFileName(
                this, QObject::tr("Run Script"), QString(AZ::Utils::GetProjectPath().c_str()), QString("*.py"));
            if (!script.isEmpty())
            {
                QTimer::singleShot(0, [script]() {
                    AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                        &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, script.toUtf8().constData());
                });
            }
        });

        m_menuFile->addSeparator();

        m_menuFile->addAction("E&xit", [this]() {
            close();
        }, QKeySequence::Quit);

        // Add menu options to toggle the visibility of all dock widgets
        auto dockWidgets = findChildren<QDockWidget*>();
        AZStd::sort(dockWidgets.begin(), dockWidgets.end(), [](QDockWidget* a, QDockWidget* b) {
            return a->windowTitle() < b->windowTitle();
        });

        for (auto dockWidget : dockWidgets)
        {
            const auto dockWidgetName = dockWidget->windowTitle();
            auto dockAction = m_menuTools->addAction(dockWidgetName, [this, dockWidgetName](const bool checked) {
                SetDockWidgetVisible(dockWidgetName.toUtf8().constData(), checked);
            });
            dockAction->setCheckable(true);
            dockAction->setChecked(dockWidget->isVisible());
            connect(dockWidget, &QDockWidget::visibilityChanged, dockAction, &QAction::setChecked);
        }

        m_menuTools->addSeparator();
        m_menuTools->addAction("Default Layout", [this]() {
            m_advancedDockManager->restoreState(m_defaultWindowState);
        });
        m_menuTools->addSeparator();

        m_menuTools->addAction("&Settings...", [this]() {
            OpenSettings();
        }, QKeySequence::Preferences);

        m_menuHelp->addAction("&Help...", [this]() {
            OpenHelp();
        });

        m_menuHelp->addAction("&About...", [this]() {
            OpenAbout();
        });
    }

    void AtomToolsMainWindow::UpdateMenus([[maybe_unused]] QMenuBar* menuBar)
    {
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

    void AtomToolsMainWindow::OpenSettings()
    {
    }

    void AtomToolsMainWindow::OpenHelp()
    {
    }

    void AtomToolsMainWindow::OpenAbout()
    {
        QMessageBox::about(this, windowTitle(), QApplication::applicationName());
    }

    void AtomToolsMainWindow::showEvent(QShowEvent* showEvent)
    {
        if (!m_shownBefore)
        {
            m_shownBefore = true;
            m_defaultWindowState = m_advancedDockManager->saveState();
            m_mainWindowWrapper->showFromSettings();
            const AZStd::string windowState =
                AtomToolsFramework::GetSettingsObject("/O3DE/AtomToolsFramework/MainWindow/WindowState", AZStd::string());
            m_advancedDockManager->restoreState(QByteArray(windowState.data(), aznumeric_cast<int>(windowState.size())));
        }

        Base::showEvent(showEvent);
    }

    void AtomToolsMainWindow::closeEvent(QCloseEvent* closeEvent)
    {
        if (closeEvent->isAccepted())
        {
            const QByteArray windowState = m_advancedDockManager->saveState();
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/AtomToolsFramework/MainWindow/WindowState", AZStd::string(windowState.begin(), windowState.end()));
            AtomToolsMainWindowNotificationBus::Event(m_toolId, &AtomToolsMainWindowNotifications::OnMainWindowClosing);
        }

        Base::closeEvent(closeEvent);
    }

    void AtomToolsMainWindow::SetupMetrics()
    {
        m_statusBarCpuTime = new QLabel(this);
        statusBar()->addPermanentWidget(m_statusBarCpuTime);
        m_statusBarGpuTime = new QLabel(this);
        statusBar()->addPermanentWidget(m_statusBarGpuTime);
        m_statusBarFps = new QLabel(this);
        statusBar()->addPermanentWidget(m_statusBarFps);

        static constexpr int UpdateIntervalMs = 1000;
        m_metricsTimer.setInterval(UpdateIntervalMs);
        m_metricsTimer.start();
        connect(&m_metricsTimer, &QTimer::timeout, this, &AtomToolsMainWindow::UpdateMetrics);

        PerformanceMonitorRequestBus::Broadcast(&PerformanceMonitorRequestBus::Handler::SetProfilerEnabled, true);

        UpdateMetrics();
    }

    void AtomToolsMainWindow::UpdateMetrics()
    {
        PerformanceMetrics metrics = {};
        PerformanceMonitorRequestBus::BroadcastResult(metrics, &PerformanceMonitorRequestBus::Handler::GetMetrics);

        m_statusBarCpuTime->setText(tr("CPU Time %1 ms").arg(QString::number(metrics.m_cpuFrameTimeMs, 'f', 2)));
        m_statusBarGpuTime->setText(tr("GPU Time %1 ms").arg(QString::number(metrics.m_gpuFrameTimeMs, 'f', 2)));
        int frameRate = metrics.m_cpuFrameTimeMs > 0 ? aznumeric_cast<int>(1000 / metrics.m_cpuFrameTimeMs) : 0;
        m_statusBarFps->setText(tr("FPS %1").arg(QString::number(frameRate)));
    }

    void AtomToolsMainWindow::UpdateWindowTitle()
    {
        AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            QString title = QString{ "%1 (%2)" }.arg(QApplication::applicationName()).arg(apiName.GetCStr());
            setWindowTitle(title);
        }
        else
        {
            AZ_Assert(false, "Render API name not found");
            setWindowTitle(QApplication::applicationName());
        }
    }
} // namespace AtomToolsFramework
