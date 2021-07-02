/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "DrillerMainWindow.hxx"
#include <Source/Driller/moc_DrillerMainWindow.cpp>

#include "DrillerCaptureWindow.hxx"

#include "DrillerMainWindowMessages.h"
#include "DrillerAggregator.hxx"
#include "ChannelControl.hxx"
#include "CombinedEventsControl.hxx"
#include "DrillerDataContainer.h"
#include "Workspaces/Workspace.h"

#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <AzToolsFramework/UI/LegacyFramework/CustomMenus/CustomMenusAPI.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <QtGui/QPalette>

#include <AzCore/std/sort.h>

#include <Source/Driller/ui_DrillerMainWindow.h>

#include <QTimer>
#include <QTabBar>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QFileDialog>
#include <QFileInfo>

namespace Driller::MainWindow
{
    const char* drillerDebugName = "Driller";
}

namespace Driller
{
    class DrillerMainWindowSavedState
        : public AzToolsFramework::MainWindowSavedState
    {
    public:
        AZ_RTTI(DrillerMainWindowSavedState, "{77A8D5DB-38EB-4F9B-BEA2-F42D725A8177}", AzToolsFramework::MainWindowSavedState);
        AZ_CLASS_ALLOCATOR(DrillerMainWindowSavedState, AZ::SystemAllocator, 0);

        AZStd::string m_priorSaveFolder;
        AZStd::string m_priorOpenFolder;

        DrillerMainWindowSavedState() {}

        void Init(const QByteArray& windowState, const QByteArray& windowGeom)
        {
            AzToolsFramework::MainWindowSavedState::Init(windowState, windowGeom);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<DrillerMainWindowSavedState, AzToolsFramework::MainWindowSavedState    >()
                    ->Field("m_priorSaveFolder", &DrillerMainWindowSavedState::m_priorSaveFolder)
                    ->Field("m_priorOpenFolder", &DrillerMainWindowSavedState::m_priorOpenFolder)
                    ->Version(8);
            }
        }
    };

    // WORKSPACES are files loaded and stored independent of the global application
    // designed to be used for DRL data specific view settings and to pass around
    class DrillerMainWindowWorkspace
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(DrillerMainWindowWorkspace, "{E7DAC981-84E9-490E-AF1B-DADC116B3B10}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(DrillerMainWindowWorkspace, AZ::SystemAllocator, 0);

        AZStd::vector<AZStd::string> m_openDataFileNames;

        DrillerMainWindowWorkspace() {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<DrillerMainWindowWorkspace>()
                    ->Field("m_openDataFileNames", &DrillerMainWindowWorkspace::m_openDataFileNames)
                    ->Version(8);
            }
        }
    };
}

namespace Driller
{
    extern AZ::Uuid ContextID;

    Driller::DrillerMainWindow* s_drillerMainWindowScriptPtr = NULL; // for script access

    int DrillerMainWindow::m_ascendingIdentity = 0;

    //////////////////////////////////////////////////////////////////////////
    //DrillerMainWindow
    DrillerMainWindow::DrillerMainWindow(QWidget* parent, Qt::WindowFlags flags)
        : QMainWindow(parent, flags)
    {
        s_drillerMainWindowScriptPtr = this;

        m_isLoadingFile = false;
        m_panningMainView = false;
        m_panningMainViewStartPoint = 0;

        m_gui = azcreate(Ui::DrillerMainWindow, ());
        m_gui->setupUi(this);

        QMenu* theMenu = new QMenu(this);
        (void)theMenu->addAction(
            "Close Profiler App",
            this,
            SLOT(OnMenuCloseCurrentWindow()),
            QKeySequence("Alt+F4")
        );

        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, PopulateApplicationMenu, theMenu);
        menuBar()->insertMenu(m_gui->menuDriller->menuAction(), theMenu);

        connect(m_gui->actionContract, SIGNAL(triggered()), this, SLOT(OnContractAllChannels()));
        connect(m_gui->actionExpand, SIGNAL(triggered()), this, SLOT(OnExpandAllChannels()));
        connect(m_gui->actionDisable, SIGNAL(triggered()), this, SLOT(OnDisableAllChannels()));
        connect(m_gui->actionEnable, SIGNAL(triggered()), this, SLOT(OnEnableAllChannels()));

        DrillerDataViewMessages::Handler::BusConnect();

        QTimer::singleShot(0, this, SLOT(RestoreWindowState()));

        AzFramework::TargetManagerClient::Bus::Handler::BusConnect();

        m_gui->actionSave->setEnabled(false);
        m_gui->actionSaveWorkspace->setEnabled(true);

        // default identity == 0 Live tab
        auto captureWindow = aznew DrillerCaptureWindow(CaptureMode::Configuration, m_ascendingIdentity, this);
        captureWindow->setAttribute(Qt::WA_DeleteOnClose, true);

        m_captureWindows.insert(AZStd::make_pair(captureWindow, m_ascendingIdentity));
        ++m_ascendingIdentity;

        m_gui->tabbedContents->addTab(captureWindow, QString("LIVE"));

        // Enabling close buttons on our widgets, and hiding the close button on the live
        // tab for now.
        m_gui->tabbedContents->setTabsClosable(true);
        m_gui->tabbedContents->tabBar()->setTabButton(0, QTabBar::ButtonPosition::RightSide, nullptr);
        m_gui->tabbedContents->tabBar()->setTabButton(0, QTabBar::ButtonPosition::LeftSide, nullptr);

        connect(m_gui->tabbedContents, SIGNAL(currentChanged(int)), this, SLOT(OnTabChanged(int)));
        connect(m_gui->tabbedContents, SIGNAL(tabCloseRequested(int)), this, SLOT(CloseTab(int)));

        connect(captureWindow, SIGNAL(destroyed(QObject*)), this, SLOT(OnCaptureWindowDestroyed(QObject*)));

        EBUS_EVENT(LegacyFramework::CustomMenusMessages::Bus, RegisterMenu, LegacyFramework::CustomMenusCommon::Driller::Application, theMenu);
        EBUS_EVENT(LegacyFramework::CustomMenusMessages::Bus, RegisterMenu, LegacyFramework::CustomMenusCommon::Driller::DrillerMenu, m_gui->menuDriller);
        EBUS_EVENT(LegacyFramework::CustomMenusMessages::Bus, RegisterMenu, LegacyFramework::CustomMenusCommon::Driller::Channels, m_gui->menuChannels);

        UpdateTabBarDisplay();
    }

    DrillerMainWindow::~DrillerMainWindow(void)
    {
        AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();

        s_drillerMainWindowScriptPtr = NULL;
        DrillerDataViewMessages::Handler::BusDisconnect();

        azdestroy(m_gui);
    }

    void DrillerMainWindow::OnTabChanged(int toWhich)
    {
        // first tab is always Live
        if (toWhich)
        {
            m_gui->actionSave->setEnabled(true);
        }
        else
        {
            m_gui->actionSave->setEnabled(false);
        }
    }

    void DrillerMainWindow::CloseTab(int closeTab)
    {
        // We never want to close the live tab.
        if (closeTab != 0)
        {
            DrillerCaptureWindow* captureWindow = static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->widget(closeTab));

            if (captureWindow)
            {
                captureWindow->OnClose();
            }
        }
    }

    void DrillerMainWindow::OnCaptureWindowDestroyed(QObject* cWindow)
    {
        auto captureWindow = static_cast<DrillerCaptureWindow*>(cWindow);
        m_gui->tabbedContents->removeTab(m_gui->tabbedContents->indexOf(captureWindow));
        m_captureWindows.erase(captureWindow);

        UpdateTabBarDisplay();
    }

    //////////////////////////////////////////////////////////////////////////
    // Data Viewer Request Messages
    void DrillerMainWindow::EventRequestOpenFile(AZStd::string fileName)
    {
        OnOpenDrillerFile(fileName.c_str());
    }
    void DrillerMainWindow::EventRequestOpenWorkspace(AZStd::string fileName)
    {
        OnOpenWorkspaceFile(fileName.c_str(), true);
    }

    //////////////////////////////////////////////////////////////////////////
    // GUI Messages
    void DrillerMainWindow::OnMenuCloseCurrentWindow()
    {
        AZ_TracePrintf(Driller::MainWindow::drillerDebugName, "Close requested\n");

        SaveWindowState();

        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, RequestMainWindowClose, ContextID);
    }

    void DrillerMainWindow::OnOpen()
    {
        AZ_TracePrintf(Driller::MainWindow::drillerDebugName, "Open requested\n");

        this->show();
        emit ShowYourself();
    }

    void DrillerMainWindow::OnClose()
    {
        AZ_TracePrintf(Driller::MainWindow::drillerDebugName, "Close requested of window (not file)\n");
    }

    void DrillerMainWindow::OnContractAllChannels()
    {
        static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->currentWidget())->OnContractAllChannels();
    }
    void DrillerMainWindow::OnExpandAllChannels()
    {
        static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->currentWidget())->OnExpandAllChannels();
    }
    void DrillerMainWindow::OnDisableAllChannels()
    {
        static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->currentWidget())->OnDisableAllChannels();
    }
    void DrillerMainWindow::OnEnableAllChannels()
    {
        static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->currentWidget())->OnEnableAllChannels();
    }

    //////////////////////////////////////////////////////////////////////////
    // when the Editor Main window is requested to close, it is not destroyed.
    //////////////////////////////////////////////////////////////////////////
    // Qt Events
    void DrillerMainWindow::closeEvent(QCloseEvent* event)
    {
        OnMenuCloseCurrentWindow();
        event->ignore();
    }

    void DrillerMainWindow::showEvent(QShowEvent* /*event*/)
    {
        emit ShowYourself();
    }
    void DrillerMainWindow::hideEvent(QHideEvent* /*event*/)
    {
        emit HideYourself();
    }

    bool DrillerMainWindow::OnGetPermissionToShutDown()
    {
        for (auto idx = 0; idx < m_gui->tabbedContents->count(); ++idx)
        {
            bool willShutDown = static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->widget(idx))->OnGetPermissionToShutDown();
            if (!willShutDown)
            {
                AZ_TracePrintf(Driller::MainWindow::drillerDebugName, "                            ShutDown Denied\n");
                return false;
            }
        }

        AZ_TracePrintf(Driller::MainWindow::drillerDebugName, "                            willShutDown == 1\n");
        return true;
    }

    void DrillerMainWindow::SaveWindowState()
    {
        // build state and store it.
        auto newState = AZ::UserSettings::CreateFind<DrillerMainWindowSavedState>(AZ_CRC("DRILLER MAIN WINDOW STATE", 0x9c98b7f6), AZ::UserSettings::CT_GLOBAL);
        newState->Init(saveState(), saveGeometry());

        for (auto iter = m_captureWindows.begin(); iter != m_captureWindows.end(); ++iter)
        {
            iter->first->SaveWindowState();
        }
    }

    void DrillerMainWindow::UpdateTabBarDisplay()
    {
        // We will always have one window open(live), and we don't want to show
        // the tab bar unless we have more then one.
        m_gui->tabbedContents->tabBar()->setVisible(m_captureWindows.size() > 1);
    }

    void DrillerMainWindow::RestoreWindowState() // call this after you have rebuilt everything.
    {
        // load the state from our state block:
        auto savedState = AZ::UserSettings::Find<DrillerMainWindowSavedState>(AZ_CRC("DRILLER MAIN WINDOW STATE", 0x9c98b7f6), AZ::UserSettings::CT_GLOBAL);
        if (savedState)
        {
            QByteArray geomData((const char*)savedState->m_windowGeometry.data(), (int)savedState->m_windowGeometry.size());
            QByteArray stateData((const char*)savedState->GetWindowState().data(), (int)savedState->GetWindowState().size());

            restoreGeometry(geomData);
            if (this->isMaximized())
            {
                this->showNormal();
                this->showMaximized();
            }
            restoreState(stateData);
        }
        else
        {
            // default state!
        }
    }

    void DrillerMainWindow::OnOpenDrillerFile()
    {
        QString capturePath;

        auto newState = AZ::UserSettings::CreateFind<DrillerMainWindowSavedState>(AZ_CRC("DRILLER MAIN WINDOW STATE", 0x9c98b7f6), AZ::UserSettings::CT_GLOBAL);
        if (!newState->m_priorOpenFolder.empty())
        {
            capturePath = newState->m_priorOpenFolder.data();
        }
        else
        {
            capturePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

            if (capturePath.isEmpty())
            {
                capturePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            }
        }

        QString fileName = QFileDialog::getOpenFileName(this, "Open Driller File", capturePath, "Driller Files (*.drl)");
        if (!fileName.isNull())
        {
            OnOpenDrillerFile(fileName);
            newState->m_priorOpenFolder = QFileInfo(fileName).dir().canonicalPath().toUtf8().data();
        }
    }

    void DrillerMainWindow::OnOpenDrillerFile(QString fileName)
    {
        auto captureWindow = aznew DrillerCaptureWindow(CaptureMode::Inspecting, m_ascendingIdentity, this);
        if (captureWindow)
        {
            m_captureWindows.insert(AZStd::make_pair(captureWindow, m_ascendingIdentity));
            ++m_ascendingIdentity;

            captureWindow->OnOpenDrillerFile(fileName);
            connect(captureWindow, SIGNAL(destroyed(QObject*)), this, SLOT(OnCaptureWindowDestroyed(QObject*)));

            m_gui->tabbedContents->setCurrentIndex(m_gui->tabbedContents->addTab(captureWindow, fileName));
            UpdateTabBarDisplay();
        }
    }

    void DrillerMainWindow::OnOpenWorkspaceFile()
    {
        QString capturePath;

        auto newState = AZ::UserSettings::CreateFind<DrillerMainWindowSavedState>(AZ_CRC("DRILLER MAIN WINDOW STATE", 0x9c98b7f6), AZ::UserSettings::CT_GLOBAL);
        if (!newState->m_priorOpenFolder.empty())
        {
            capturePath = newState->m_priorOpenFolder.data();
        }
        else
        {
            capturePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (capturePath.isEmpty())
            {
                capturePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            }
        }

        QString workspaceFileName = QFileDialog::getOpenFileName(this, tr("Open Workspace File"), capturePath, tr("Workspace Files (*.drw)"));
        if (!workspaceFileName.isNull())
        {
            OnOpenWorkspaceFile(workspaceFileName, true);
        }
    }

    void DrillerMainWindow::OnOpenWorkspaceFile(QString workspaceFileName, bool openDrillerFileAlso)
    {
        auto captureWindow = aznew DrillerCaptureWindow(CaptureMode::Inspecting, m_ascendingIdentity, this);
        if (captureWindow)
        {
            m_captureWindows.insert(AZStd::make_pair(captureWindow, m_ascendingIdentity));
            ++m_ascendingIdentity;

            captureWindow->OnOpenWorkspaceFile(workspaceFileName, openDrillerFileAlso);
            connect(captureWindow, SIGNAL(destroyed(QObject*)), this, SLOT(OnCaptureWindowDestroyed(QObject*)));

            m_gui->tabbedContents->setCurrentIndex(m_gui->tabbedContents->addTab(captureWindow, captureWindow->GetDataFileName()));
            UpdateTabBarDisplay();
        }
    }

    void DrillerMainWindow::OnApplyWorkspaceFile()
    {
        QString capturePath;

        auto newState = AZ::UserSettings::CreateFind<DrillerMainWindowSavedState>(AZ_CRC("DRILLER MAIN WINDOW STATE", 0x9c98b7f6), AZ::UserSettings::CT_GLOBAL);
        if (!newState->m_priorOpenFolder.empty())
        {
            capturePath = newState->m_priorOpenFolder.data();
        }
        else
        {
            capturePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (capturePath.isEmpty())
            {
                capturePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            }
        }

        QString fileName = QFileDialog::getOpenFileName(this, "Apply Workspace", capturePath, "Workspace Files (*.drw)");
        if (!fileName.isNull())
        {
            static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->currentWidget())->OnApplyWorkspaceFile(fileName);
        }
    }

    void DrillerMainWindow::OnSaveWorkspaceFile()
    {
        QString capturePath;

        auto newState = AZ::UserSettings::CreateFind<DrillerMainWindowSavedState>(AZ_CRC("DRILLER MAIN WINDOW STATE", 0x9c98b7f6), AZ::UserSettings::CT_GLOBAL);
        if (!newState->m_priorOpenFolder.empty())
        {
            capturePath = newState->m_priorOpenFolder.data();
        }
        else
        {
            capturePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (capturePath.isEmpty())
            {
                capturePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            }
        }

        QString fileName = QFileDialog::getSaveFileName(this, "Save Workspace", capturePath, "Workspace Files (*.drw)");
        if (!fileName.isNull())
        {
            static_cast<DrillerCaptureWindow*>(m_gui->tabbedContents->currentWidget())->OnSaveWorkspaceFile(fileName);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Target Manager Messages
    void DrillerMainWindow::Reflect(AZ::ReflectContext* context)
    {
        // data container is the one place that knows about all the aggregators
        // and indeed is responsible for creating them
        Driller::WorkspaceSettingsProvider::Reflect(context);
        DrillerMainWindowWorkspace::Reflect(context);
        DrillerMainWindowSavedState::Reflect(context);
        DrillerCaptureWindow::Reflect(context);

        // reflect data for script, serialization, editing...
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<DrillerMainWindow>("DrillerMainWindow")->
                Method("ShowWindow", &DrillerMainWindow::OnOpen)->
                Method("HideWindow", &DrillerMainWindow::OnClose);

            behaviorContext->Property("DrillerMainWindow", BehaviorValueGetter(&s_drillerMainWindowScriptPtr), nullptr);
        }
    }
}//namespace Driller
