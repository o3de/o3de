/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QtGui>
#include <QtWidgets/QMenu>
#include <QtNetwork/QHostInfo>
AZ_POP_DISABLE_WARNING

#ifdef AZ_PLATFORM_WINDOWS
#include <AzCore/PlatformIncl.h>
#else
#include <signal.h>
#endif

#include "gridhub.hxx"
#include <GridHub/moc_gridhub.cpp>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GridMate/Carrier/Utils.h>
#include <GridMate/Session/LANSession.h>

#include <time.h>   // for output timestamp

//////////////////////////////////////////////////////////////////////////


GridHub::GridHub(AZ::ComponentApplication* componentApp, GridHubComponent* hubComponent, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
    , m_componentApp(componentApp)
    , m_hubComponent(hubComponent)
{
    // create actions
    //m_minimizeAction = new QAction(tr("Mi&nimize"), this);
    //connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    //m_maximizeAction = new QAction(tr("Ma&ximize"), this);
    //connect(m_maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    m_restoreAction = new QAction(tr("&Show"), this);
    connect(m_restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    m_quitAction = new QAction(tr("&Quit"), this);
    connect(m_quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    // create menu
    m_trayIconMenu = new QMenu(this);
    //m_trayIconMenu->addAction(m_minimizeAction);
    //m_trayIconMenu->addAction(m_maximizeAction);
    m_trayIconMenu->addAction(m_restoreAction);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_quitAction);

    // create tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->setToolTip(QString("Amazon Debug Connection Hub - GridHub"));
    m_trayIcon->setIcon(QIcon(":/GridHub/Resources/Disconnected.png"));
    
    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),   this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    m_trayIcon->show();
    
    setWindowFlags(Qt::Dialog);
    ui.setupUi(this);

    QObject::connect(ui.startStopService,SIGNAL(clicked()),this,SLOT(OnStartStopSession()));

    ui.sessionPort->setValue(m_hubComponent->GetSessionPort());
    QObject::connect(ui.sessionPort, SIGNAL(valueChanged(int)),this, SLOT(SetSessionPort(int)));
    ui.numSlots->setValue(m_hubComponent->GetSessionSlots());
    QObject::connect(ui.numSlots, SIGNAL(valueChanged(int)),this, SLOT(SetSessionSlots(int)));
    ui.hubName->setText(QString(m_hubComponent->GetHubName().c_str()));
    QObject::connect(ui.hubName, SIGNAL(textChanged(QString)),this, SLOT(SetHubName(QString)));
    ui.isDisconnectDetect->setChecked(m_hubComponent->IsEnableDisconnectDetection());
    QObject::connect(ui.isDisconnectDetect, SIGNAL(stateChanged(int)),this,SLOT(EnableDisconnectDetection(int)));
    ui.isAddToStartup->setChecked(m_hubComponent->IsAddToStartupFolder());
    QObject::connect(ui.isAddToStartup, SIGNAL(stateChanged(int)),this,SLOT(AddToStartupFolder(int)));
    ui.isLogToFile->setChecked(m_hubComponent->IsLogToFile());
    QObject::connect(ui.isLogToFile, SIGNAL(stateChanged(int)),this,SLOT(LogToFile(int)));
    ui.disconnectionTimeout->setValue(m_hubComponent->GetDisconnectionTimeout());
    QObject::connect(ui.disconnectionTimeout, SIGNAL(valueChanged(int)),this,SLOT(OnDisconnectTimeOutChange(int)));

    SanityCheckDetectionTimeout();
    
    // start at XX ms update interval
    startTimer(30);

    AZ::Debug::TraceMessageBus::Handler::BusConnect();

    AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

    // start the session
    OnStartStopSession();

    // Kicks off the tool's tick event
    SystemTick();
}

GridHub::~GridHub()
{
    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

    if (m_hubComponent)
    {
        m_hubComponent->StopSession();
    }

    AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();

    delete m_trayIcon;
    delete m_trayIconMenu;
    delete m_quitAction;
    delete m_restoreAction;
    //delete m_maximizeAction;
    //delete m_minimizeAction;
}

void GridHub::SystemTick()
{
    AZ::SystemTickBus::ExecuteQueuedEvents();
    AZ::SystemTickBus::Broadcast(&AZ::SystemTickEvents::OnSystemTick);

    QTimer::singleShot(10, Qt::PreciseTimer, this, &GridHub::SystemTick);
}


//=========================================================================
// closeEvent
// [7/6/2012]
//=========================================================================
void GridHub::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon->isVisible()) {
/*      QMessageBox::information(this, tr("GridHub - Amazon Debug Connection Hub"),
            tr("The program will keep running in the "
            "system tray. To terminate the program, "
            "choose <b>Quit</b> in the context menu "
            "that pops up when clicking this program's "
            "entry in the system tray."));*/
        hide();
        event->ignore();
    }
}

void GridHub::SanityCheckDetectionTimeout()
{
    ui.disconnectionTimeout->setEnabled(m_hubComponent->IsEnableDisconnectDetection());
}

//=========================================================================
// timerEvent
// [7/6/2012]
//=========================================================================
void GridHub::timerEvent([[maybe_unused]] QTimerEvent *event)
{
    m_componentApp->Tick();

    if( !m_hubComponent->IsInSession() )
    {
        // if we get an address 169.X.X.X (AZCP is NOT ready) or 127.0.0.1 when network is not ready
        AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
        static AZStd::chrono::system_clock::time_point lastUpdate = now;
        if( AZStd::chrono::seconds(now - lastUpdate).count() >= 10 )
        {
            // Try to start it again...
            OnStartStopSession();
            lastUpdate = now;
        }
    }

    UpdateOutput();

    UpdateMembers();
}

//=========================================================================
// UpdateOutput
// [5/24/2011]
//=========================================================================
void GridHub::UpdateOutput()
{
    if( m_output.empty() ) return;
    m_outputMutex.lock();
    QString msg(m_output.c_str());
    m_output.clear();
    m_outputMutex.unlock();

    if(msg.endsWith('\n')) // append already inserts '\n' at the end
        msg.chop(1);
    ui.output->append(msg);
}

//=========================================================================
// UpdateMembers
// [7/11/2012]
//=========================================================================
void GridHub::UpdateMembers()
{
    // update ~1 time a sec
    AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
    static AZStd::chrono::system_clock::time_point lastUpdate = now;
    if( AZStd::chrono::milliseconds(now - lastUpdate).count() < 1000 )
        return;

    lastUpdate = now;

    const GridMate::GridSession* session = m_hubComponent->GetSession();
    if( m_hubComponent == nullptr || session == nullptr || !session->IsReady()  ) 
    {
        ui.members->setRowCount(0);
    }
    else
    {
        ui.members->setRowCount(session->GetNumberOfMembers());
        for(unsigned int i = 0; i < session->GetNumberOfMembers(); ++i)
        {
            GridMate::GridMember* member = session->GetMemberByIndex(i);
            ui.members->setItem(i,0,new QTableWidgetItem(member->GetId().ToString().c_str()));
            ui.members->setItem(i,1,new QTableWidgetItem(member->GetName().c_str()));
            ui.members->setItem(i,2,new QTableWidgetItem(member->GetConnectionId()==GridMate::InvalidConnectionID ? "--" : QString::number((size_t)member->GetConnectionId())));
            ui.members->setItem(i,3,new QTableWidgetItem(member->IsHost()?"Yes":"No"));
            ui.members->setItem(i,4,new QTableWidgetItem(member->IsLocal()?"Yes":"No"));
            ui.members->setItem(i,5,new QTableWidgetItem(member->IsReady()?"Yes":"No"));
        }
    }
}

//=========================================================================
// TraceOutput
// [5/20/2011]
//=========================================================================
bool GridHub::OnOutput(const char* window, const char* message)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss|");

    {
        // This function will be called from multiple threads
        const AZStd::lock_guard<AZStd::mutex> l(m_outputMutex);
        m_output += time.toUtf8().data();
        m_output += window;
        m_output += " : ";
        m_output += message;
    }

    return false;
}

//=========================================================================
// OnStartStopSession
// [7/6/2012]
//=========================================================================
void GridHub::OnStartStopSession()
{
    if( m_hubComponent->IsInSession() )
    {
        m_hubComponent->StopSession();
        ui.startStopService->setText("Start");
        m_trayIcon->setIcon(QIcon(":/GridHub/Resources/Disconnected.png"));
    }
    else
    {
        if( m_hubComponent->StartSession() )
        {
            ui.startStopService->setText("Stop");
            m_trayIcon->setIcon(QIcon(":/GridHub/Resources/Connected.png"));
        }
    }
}

//=========================================================================
// SetSessionPort
// [7/11/2012]
//=========================================================================
void GridHub::SetSessionPort(int port)
{
    m_hubComponent->SetSessionPort(static_cast<unsigned short>(port));
}

//=========================================================================
// SetSessionSlots
// [7/11/2012]
//=========================================================================
void GridHub::SetSessionSlots(int numberOfSlots)
{
    m_hubComponent->SetSessionSlots(static_cast<unsigned char>(numberOfSlots));
}

//=========================================================================
// EnableDisconnectDetection
// [9/14/2012]
//=========================================================================
void GridHub::EnableDisconnectDetection(int state)
{
    m_hubComponent->EnableDisconnectDetection(state != 0);

    SanityCheckDetectionTimeout();
}

//=========================================================================
// EnableDisconnectDetection
// [10/2/2012]
//=========================================================================
void GridHub::AddToStartupFolder(int state)
{
    m_hubComponent->AddToStartupFolder(state != 0);
}

//=========================================================================
// LogToFile
// [7/2/2013]
//=========================================================================
void GridHub::LogToFile(int state)
{
    m_hubComponent->LogToFile(state != 0);
}

void GridHub::OnDisconnectTimeOutChange(int value)
{
    m_hubComponent->SetDisconnectionTimeOut(value);
}

//=========================================================================
// SetHubName
// [9/6/2012]
//=========================================================================
void GridHub::SetHubName(const QString & name)
{
    m_hubComponent->SetHubName(AZStd::string(name.toUtf8().constData()));
}
//////////////////////////////////////////////////////////////////////////
//

//=========================================================================
// GridHubComponent
// [7/6/2012]
//=========================================================================
GridHubComponent::GridHubComponent() 
    : m_ui(nullptr)
    , m_gridMate(nullptr)
    , m_session(nullptr)
{
    m_sessionPort = 5172;
    m_numberOfSlots = 10;
    m_isDisconnectDetection = true;
    m_disconnectionTimeout = 5000;
    m_isAddToStartupFolder = false;
    m_isLogToFile = false;
    
#ifdef AZ_PLATFORM_WINDOWS
    wchar_t name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwCompNameLen = AZ_ARRAY_SIZE(name);
    if (GetComputerName(name, &dwCompNameLen) != 0) 
    {
        AZStd::to_string(m_hubName, name);
    }
    else
#endif
        m_hubName = QHostInfo::localHostName().toUtf8().data();
}

//=========================================================================
// ~GridHubComponent
// [7/6/2012]
//=========================================================================
GridHubComponent::~GridHubComponent()
{
}

//=========================================================================
// Init
// [7/6/2012]
//=========================================================================
void GridHubComponent::Init()
{
}

//=========================================================================
// Activate
// [7/6/2012]
//=========================================================================
void GridHubComponent::Activate()
{
    AZ::Debug::TraceMessageBus::Handler::BusConnect();

}

//=========================================================================
// Deactivate
// [7/6/2012]
//=========================================================================
void GridHubComponent::Deactivate()
{
    if( IsInSession() )
        StopSession();
    
    AZ_Assert(m_monitored.empty(),"We should have removed all monitored members already!");

    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
}

//=========================================================================
// OnSystemTick
//=========================================================================
void GridHubComponent::OnSystemTick()
{
    if( m_gridMate )
    {
        m_gridMate->Update();

        if( m_session == nullptr ) 
        {
            // It does have in certain condition that the PC was locked and we did not receive OnTick for a long time
            // GridMate will detect that and delete the session in the update. If that happens we should just restart the entire gridmate.
            RestartSession();
        }
        else
        {
            if (m_session->GetReplicaMgr())
            {
                m_session->GetReplicaMgr()->Unmarshal();
                m_session->GetReplicaMgr()->UpdateFromReplicas();
                m_session->GetReplicaMgr()->UpdateReplicas();
                m_session->GetReplicaMgr()->Marshal();
            }

            if( !m_session->DebugIsEnableDisconnectDetection() )
            {
                AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
                static AZStd::chrono::system_clock::time_point lastUpdate = now;
                if( AZStd::chrono::milliseconds(now - lastUpdate).count() >= 1000)
                {
                    lastUpdate = now;

                    // when we have disconnect detection off, check if the process terminated
                    ExternalProcessMonitor* memberToKick = nullptr;
                    for(size_t i = 0; i < m_monitored.size(); ++i)
                    {
                        ExternalProcessMonitor& mi =  m_monitored[i];
#ifdef AZ_PLATFORM_WINDOWS
                        if( mi.m_localProcess != INVALID_HANDLE_VALUE )
                        {
                            DWORD exitCode = 0;
                            GetExitCodeProcess(mi.m_localProcess,&exitCode);
                            if( exitCode != STILL_ACTIVE )
                            {
                                memberToKick = &mi;
                                break;
                            }
                        }
#else
                        if (mi.m_localProcess != 0)
                        {
                            if (kill(mi.m_localProcess, 0) != 0)
                            {
                                memberToKick = &mi;
                                break;
                            }
                        }
#endif
                    }

                    if( memberToKick )
                    {
                        GridMate::GridMember* member = m_session->GetMemberById(memberToKick->m_memberId);
                        if( member )
                        {
                            AZ_Printf("GridHub","Kicking member %s due to process inactivity!\n",member->GetId().ToString().c_str());
                            m_session->KickMember(member);
                        }
                    }
                }
            }
        }
    }
}

//=========================================================================
// LogToFile
// [7/2/2013]
//=========================================================================
void GridHubComponent::LogToFile(bool enable)
{ 
    if( m_isLogToFile && !enable )
    {
        AZ_Printf("GridHub","Logging stopped!"); // indicate in the log that the user stopped logging.
    }
    m_isLogToFile = enable; 
}

//=========================================================================
// OnOutput
// [7/2/2013]
//=========================================================================
bool GridHubComponent::OnOutput(const char* window, const char* message)
{
    if( m_isLogToFile )
    {
        if( !m_logFile.IsOpen() )
        {
            const char* logFileName = "GridHubEvents.log"; // we can make this user configurable
            if( !m_logFile.Open(logFileName,AZ::IO::SystemFile::SF_OPEN_APPEND) )
            {
                m_logFile.Open(logFileName,AZ::IO::SystemFile::SF_OPEN_CREATE);
            }
        }

        if( m_logFile.IsOpen() )
        {
            const QByteArray time = QDateTime::currentDateTime().toString("MM:dd:yy-HH:mm:ss|").toUtf8();
            m_logFile.Write(time.data(),time.size());
            m_logFile.Write(window,strlen(window));
            m_logFile.Write(" : ", 3);
            m_logFile.Write(message,strlen(message));
        }
    }

    return false;
}
//=========================================================================
// OnMemberJoined
// [5/25/2011]
//=========================================================================
void 
GridHubComponent::OnMemberJoined([[maybe_unused]] GridMate::GridSession* session, GridMate::GridMember* member)
{
    if( member->IsLocal() ) return; 

    // add non local member for process monitoring.
    GridMate::MemberIDCompact id = member->GetId().Compact();
    switch( member->GetPlatformId() )
    {
    case AZ::PlatformID::PLATFORM_WINDOWS_64:
    case AZ::PlatformID::PLATFORM_APPLE_MAC:
        {
            AZStd::string localMachineName = GridMate::Utils::GetMachineAddress();
            if( member->GetMachineName() == localMachineName )
            {
                ExternalProcessMonitor mi;
                mi.m_memberId = id;
#ifdef AZ_PLATFORM_WINDOWS
                mi.m_localProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,TRUE,member->GetProcessId());
                if( mi.m_localProcess != INVALID_HANDLE_VALUE )
                    m_monitored.push_back(mi);
#else
                mi.m_localProcess = member->GetProcessId();
                m_monitored.push_back(mi);
#endif
            }
        }break;
    }
}

//=========================================================================
// OnMemberLeaving
// [5/25/2011]
//=========================================================================
void 
GridHubComponent::OnMemberLeaving([[maybe_unused]] GridMate::GridSession* session, GridMate::GridMember* member)
{
    GridMate::MemberIDCompact id = member->GetId().Compact();
    for(size_t i = 0; i < m_monitored.size(); ++i )
    {
        ExternalProcessMonitor& mi =  m_monitored[i];
        if( mi.m_memberId == id )
        {
#ifdef AZ_PLATFORM_WINDOWS
            if( mi.m_localProcess != INVALID_HANDLE_VALUE )
                CloseHandle(mi.m_localProcess);
#endif

            m_monitored.erase(m_monitored.begin()+i);
            break;
        }
    }
}

//=========================================================================
// OnSessionCreated
// [5/24/2011]
//=========================================================================
void
GridHubComponent::OnSessionCreated(GridMate::GridSession* session)
{
    AZ_Assert(m_session==session,"Session missmatch!");
    m_session = session;
}

//=========================================================================
// OnSessionDelete
// [5/24/2011]
//=========================================================================
void 
GridHubComponent::OnSessionDelete([[maybe_unused]] GridMate::GridSession* session)
{
    AZ_Assert(m_session==session,"Session missmatch!");
    m_session = nullptr;
}

//=========================================================================
// StartSession
// [7/6/2012]
//=========================================================================
bool GridHubComponent::StartSession(bool isRestarting)
{
    AZ_Assert(m_gridMate==nullptr,"Session was ALREADY started!");
    m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
    AZ_Assert(m_gridMate,"Failed to create a gridmate instance!");
    
    // Connect for session events
    GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);
    if (!isRestarting)
    {
        AZ::SystemTickBus::Handler::BusConnect();
    }
    
    // start the multiplayer service (session mgr, extra allocator, etc.)
    GridMate::StartGridMateService<GridMate::LANSessionService>(m_gridMate, GridMate::SessionServiceDesc());
    AZ_Assert(GridMate::HasGridMateService<GridMate::LANSessionService>(m_gridMate), "Failed to start multiplayer service for LAN!");

    // if we get an address 169.X.X.X (AZCP is NOT ready) or 127.0.0.1 when network is not ready
    AZStd::string machineIP = GridMate::Utils::GetMachineAddress();
    if( machineIP == "127.0.0.1" || machineIP.compare(0,4,"169.") == 0 )
    {
        AZ_Warning("GridHub", false, "\nCurrent IP %s might be invalid.\n",machineIP.c_str());
    }
    
    GridMate::CarrierDesc carrierDesc;
    //carrierDesc.m_port = 0; 
    carrierDesc.m_enableDisconnectDetection = m_isDisconnectDetection;    
    carrierDesc.m_driverIsCrossPlatform = true;
    carrierDesc.m_connectionTimeoutMS = m_disconnectionTimeout;
    //carrierDesc.m_driverIsFullPackets = true;

    // host session 
    GridMate::LANSessionParams sp;
    sp.m_topology = GridMate::ST_PEER_TO_PEER;
    
    // Until an authentication connection can be established between peers only supporting
    // local connections (i.e. binding to localhost).
    sp.m_address = "127.0.0.1";
    carrierDesc.m_address = "127.0.0.1";

    sp.m_numPublicSlots = m_numberOfSlots;
    sp.m_numPrivateSlots = 1;
    sp.m_port = m_sessionPort;
    sp.m_flags = 0; // no host migration support
    sp.m_numParams = 2;
    sp.m_params[0].m_id = "GridHubVersion";
    sp.m_params[0].SetValue(2);
    sp.m_params[1].m_id = "HubName";
    sp.m_params[1].SetValue(m_hubName);

    EBUS_EVENT_ID_RESULT(m_session,m_gridMate,GridMate::LANSessionServiceBus,HostSession,sp,carrierDesc);
    AZ_Assert(m_session,"Failed to host a session!");
    return true;
}

//=========================================================================
// StopSession
// [7/6/2012]
//=========================================================================
void GridHubComponent::StopSession(bool isRestarting)
{
    AZ_Assert(m_gridMate!=nullptr,"Session was NOT started!");
    
    // disconnect from events
    if (!isRestarting)
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }
    GridMate::SessionEventBus::Handler::BusDisconnect();

    m_monitored.clear();

    m_session = nullptr;

    // destroy gridmate instance
    GridMate::GridMateDestroy(m_gridMate);
    m_gridMate = nullptr;
}

//=========================================================================
// RestartSession
// [5/3/2013]
//=========================================================================
void GridHubComponent::RestartSession()
{
    AZ_TracePrintf("GridHub","GridMate restarting...");
    StopSession(true);
    StartSession(true);
}

//=========================================================================
// EnableDisconnectDetection
// [9/17/2012]
//=========================================================================
void GridHubComponent::EnableDisconnectDetection(bool en)
{
    m_isDisconnectDetection = en;
    if (m_session)
        m_session->DebugEnableDisconnectDetection(m_isDisconnectDetection);
}

//=========================================================================
// Reflect
// [7/6/2012]
//=========================================================================
void GridHubComponent::Reflect(AZ::ReflectContext* reflection)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
    if( serializeContext )
    {
        serializeContext->Class<GridHubComponent>()->
            Version(3)->
            Field("SessionPort", &GridHubComponent::m_sessionPort)->
            Field("NumberOfSlots", &GridHubComponent::m_numberOfSlots)->
            Field("HubName", &GridHubComponent::m_hubName)->
            Field("IsDisconnectDetection", &GridHubComponent::m_isDisconnectDetection)->
            Field("IsAddToStartupFolder", &GridHubComponent::m_isAddToStartupFolder)->
            Field("IsLogToFile", &GridHubComponent::m_isLogToFile)->
            Field("DisconnectionTimeOut", &GridHubComponent::m_disconnectionTimeout)
        ;
            
    }
}
