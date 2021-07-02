/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DRILLERMAINWINDOW_H
#define DRILLER_DRILLERMAINWINDOW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzToolsFramework/UI/LegacyFramework/MainWindowSavedState.h>
#include "Workspaces/Workspace.h"
#include "DrillerNetworkMessages.h"
#include "DrillerMainWindowMessages.h"

#pragma once

class QMenu;
class QAction;
class QToolbar;
class QDockWidget;
class QSettings;

#include <QtWidgets/QMainWindow>
#endif

namespace Ui
{
    class DrillerMainWindow;
}

namespace Driller
{
    class ChannelControl;
    class DrillerDataContainer;
    class CombinedEventsControl;
    class DrillerCaptureWindow;

    /*
    Driller Main Window : Now with simultaneous data sets

    Home of the real commands, channels created from external aggregators when connected, and the floating control panel.
    All inputs end up here where they are interpreted and passed downwards to all channels, to maintain consistency.
    */

    //////////////////////////////////////////////////////////////////////////
    //Main Window
    class DrillerMainWindow
        : public QMainWindow
        , public Driller::DrillerDataViewMessages::Bus::Handler
        , private AzFramework::TargetManagerClient::Bus::Handler
    {
        Q_OBJECT;
    public:
        AZ_TYPE_INFO(DrillerMainWindow, "{91E48678-AEF8-474F-BB20-DDC51ACAA43A}");
        AZ_CLASS_ALLOCATOR(DrillerMainWindow,AZ::SystemAllocator,0);
        DrillerMainWindow(QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~DrillerMainWindow(void);
        DrillerMainWindow(const DrillerMainWindow&)
        {
            // TODO: Once AZ_NO_COPY macros is in the branch in use it!
            AZ_Assert(false, "You can't copy this class!");
        }

        bool OnGetPermissionToShutDown();

    public:

        // Data Viewer request messages
        void EventRequestOpenFile(AZStd::string fileName) override;
        void EventRequestOpenWorkspace(AZStd::string fileName) override;

    public:
        // MainWindow Messages and states.
        void SaveWindowState();

    private:
        // internal workings
        void UpdateTabBarDisplay();

    protected:
        static int m_ascendingIdentity;
        AZStd::map<DrillerCaptureWindow*, int> m_captureWindows;

        // Qt Events
        virtual void closeEvent(QCloseEvent* event);
        virtual void showEvent( QShowEvent * event );
        virtual void hideEvent( QHideEvent * event );

        bool m_panningMainView;
        int m_panningMainViewStartPoint;

        QString m_tmpCaptureFilename;
        QString m_currentDataFilename;

        bool m_isLoadingFile;
        bool m_bForceNextScrub;

    public slots:
        void RestoreWindowState();
        void OnMenuCloseCurrentWindow();
        void OnOpen();
        void OnClose();
        void OnContractAllChannels();
        void OnExpandAllChannels();
        void OnDisableAllChannels();
        void OnEnableAllChannels();
        void OnOpenDrillerFile();
        void OnOpenDrillerFile(QString fileName);
        void OnOpenWorkspaceFile(); // prompt user
        void OnOpenWorkspaceFile(QString fileName, bool openDrillerFileAlso); // just open it
        void OnApplyWorkspaceFile();
        void OnSaveWorkspaceFile();
        void OnCaptureWindowDestroyed(QObject*);
        void OnTabChanged(int toWhich);
        void CloseTab(int closeTab);

signals:
        void ScrubberFrameUpdate( int frame );
        void ShowYourself();
        void HideYourself();

    public:
        static void Reflect(AZ::ReflectContext* context);

    private:

        Ui::DrillerMainWindow* m_gui;
    };
}

#endif //DRILLER_DRILLERMAINWINDOW_H
