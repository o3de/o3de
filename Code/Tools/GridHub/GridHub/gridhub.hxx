/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef GRIDHUB_H
#define GRIDHUB_H

#include <AzCore/PlatformDef.h>
#if !defined(Q_MOC_RUN)
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QWidget>
#include <QSystemTrayIcon>

#include <GridHub/ui_GridHub.h>
AZ_POP_DISABLE_WARNING

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/PlatformIncl.h>
#endif

namespace AZ
{
    class ComponentApplication;
};

class GridHubComponent;

class GridHub
    : public QWidget
    , public AZ::Debug::TraceMessageBus::Handler
{
    Q_OBJECT

public:
    GridHub(AZ::ComponentApplication* componentApp, GridHubComponent* hubComponent, QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
    ~GridHub();

public slots:
    void OnStartStopSession();
    void SetSessionPort(int port);
    void SetSessionSlots(int numberOfSlots);
    void SetHubName(const QString & name);
    void EnableDisconnectDetection(int state);
    void AddToStartupFolder(int state);
    void LogToFile(int state);
    void OnDisconnectTimeOutChange(int value);

protected:
    void SanityCheckDetectionTimeout();

    void timerEvent(QTimerEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void SystemTick();

private:
    //////////////////////////////////////////////////////////////////////////
    // TraceMessagesBus
    bool    OnOutput(const char* window, const char* message) override;
    //////////////////////////////////////////////////////////////////////////

    void UpdateOutput();
    void UpdateMembers();


    QSystemTrayIcon*    m_trayIcon;
    QMenu*              m_trayIconMenu;

    QAction *           m_restoreAction;
    QAction *           m_quitAction;
    AZ::ComponentApplication*    m_componentApp;
    GridHubComponent*    m_hubComponent;

    AZStd::string        m_output;
    AZStd::mutex        m_outputMutex;

    Ui::GridHubClass ui;
};

//////////////////////////////////////////////////////////////////////////
// TODO MOVE TO A SEPARATE FILE
#include <AzCore/Component/Component.h>
#include <GridMate/Session/Session.h>

class GridHubComponent
    : public AZ::Component
    , public AZ::SystemTickBus::Handler
    , public GridMate::SessionEventBus::Handler
    , public AZ::Debug::TraceMessageBus::Handler
{
    friend class GridHubComponentFactory;
public:
    AZ_COMPONENT(GridHubComponent, "{11E4BB35-F135-4720-A890-979195A6B74E}");

    GridHubComponent();
    ~GridHubComponent() override;

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AZ::SystemTickBus
    void OnSystemTick() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // TraceMessagesBus
    bool    OnOutput(const char* window, const char* message) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Session Events
    /// Callback that is called when the Session service is ready to process sessions.
    void OnSessionServiceReady() override {}
    /// Callback that notifies the title when a game search query have completed.
    void OnGridSearchComplete(GridMate::GridSearch* gridSearch) override { (void)gridSearch; }
    /// Callback that notifies the title when a new member joins the game session.
    void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override;
    /// Callback that notifies the title that a member is leaving the game session. member pointer is NOT valid after the callback returns.
    void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member) override;
    // \todo a better way will be (after we solve migration) is to supply a reason to OnMemberLeaving... like the member was kicked.
    // this will require that we actually remove the replica at the same moment.
    /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
    void OnMemberKicked(GridMate::GridSession* session, GridMate::GridMember* member, AZ::u8 reason) override { (void)session;(void)member;(void)reason; }
    /// After this callback it is safe to access session features. If host session is fully operational if client wait for OnSessionJoined.
    void OnSessionCreated(GridMate::GridSession* session) override;
    /// Called on client machines to indicate that we join successfully.
    void OnSessionJoined(GridMate::GridSession* session) override { (void)session; }
    /// Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
    void OnSessionDelete(GridMate::GridSession* session) override;
    /// Called when a session error occurs.
    void OnSessionError(GridMate::GridSession* session, const AZStd::string& errorMsg ) override { (void)session; (void)errorMsg; }
    /// Called when the actual game(match) starts
    void OnSessionStart(GridMate::GridSession* session) override { (void)session; }
    /// Called when the actual game(match) ends
    void OnSessionEnd(GridMate::GridSession* session) override { (void)session; }
    /// Called when we start a host migration.
    void OnMigrationStart(GridMate::GridSession* session) override { (void)session; }
    /// Called so the user can select a member that should be the new Host. Value will be ignored if NULL, current host or the member has invalid connection id.
    void OnMigrationElectHost(GridMate::GridSession* session,GridMate::GridMember*& newHost) override { (void)session;(void)newHost; }
    /// Called when the host migration has completed.
    void OnMigrationEnd(GridMate::GridSession* session,GridMate::GridMember* newHost) override { (void)session;(void)newHost; }
    //////////////////////////////////////////////////////////////////////////

    void SetUI(GridHub* ui)    { m_ui = ui; }

    bool StartSession(bool isRestarting = false);
    void StopSession(bool isRestarting = false);
    void RestartSession();
    bool IsInSession() const                        { return m_session != nullptr; }
    GridMate::GridSession*    GetSession()            { return m_session; }
    void SetSessionPort(unsigned short port)        { m_sessionPort = port; }
    int  GetSessionPort() const                        { return m_sessionPort; }
    void SetSessionSlots(unsigned char numSlots)    { m_numberOfSlots = numSlots; }
    int GetSessionSlots() const                        { return m_numberOfSlots; }
    void SetHubName(const AZStd::string& hubName)    { m_hubName = hubName; }
    void EnableDisconnectDetection(bool en);
    bool IsEnableDisconnectDetection() const        { return m_isDisconnectDetection; }
    void SetDisconnectionTimeOut(int timeout)       { m_disconnectionTimeout = timeout; }
    int GetDisconnectionTimeout() const             { return m_disconnectionTimeout; }
    void AddToStartupFolder(bool isAdd)                { m_isAddToStartupFolder = isAdd; }
    bool IsAddToStartupFolder() const                { return m_isAddToStartupFolder; }
    const AZStd::string& GetHubName() const            { return m_hubName; }

    void LogToFile(bool enable);
    bool IsLogToFile() const                        { return m_isLogToFile; }



    static void Reflect(AZ::ReflectContext* reflection);
private:

    GridHub*                m_ui;
    GridMate::IGridMate*    m_gridMate;
    GridMate::GridSession*  m_session;

    unsigned short            m_sessionPort;
    unsigned char            m_numberOfSlots;
    AZStd::string            m_hubName;
    bool                    m_isDisconnectDetection;
    int                     m_disconnectionTimeout;
    bool                    m_isAddToStartupFolder;
    bool                    m_isLogToFile;

    AZ::IO::SystemFile        m_logFile;

    /**
     * Contain information about members and titles that we monitor for exit. Only enabled if
     * we have disconnection detection off.
     */
    struct ExternalProcessMonitor
    {
        ExternalProcessMonitor()
            : m_memberId(0)
    #ifdef AZ_PLATFORM_WINDOWS
            , m_localProcess(INVALID_HANDLE_VALUE)
    #else
            , m_localProcess(0)
    #endif
        {
        }

        GridMate::MemberIDCompact    m_memberId;            ///< Pointer to member ID in the session.
#ifdef AZ_PLATFORM_WINDOWS
        HANDLE                        m_localProcess;        ///< Local process ID used to monitor local applocations.
#else
        pid_t                                           m_localProcess;
#endif

    };

    AZStd::vector<ExternalProcessMonitor> m_monitored;

};

#endif // GRIDHUB_H
