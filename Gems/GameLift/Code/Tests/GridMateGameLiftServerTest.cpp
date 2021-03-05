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
#include <GameLift/Session/GameLiftServerService.h>

#include <GridMate/Replica/ReplicaFunctions.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h>

///////////////////////////////////////////////////////////////////////////////
struct TestSettings
{
    int m_serverPort = 0;
    const char* m_logPath = ".";
};

// Test constants
static const unsigned int s_maxTicks = 1000;
static const unsigned int s_tickDelayMs = 30;

// Global GameLift settings
static TestSettings s_gameLiftSettings;
///////////////////////////////////////////////////////////////////////////////

// Handle asserts
class TraceDrillerHook
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    TraceDrillerHook()
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Create(); // used by the bus

        m_logFile.Open(AZStd::basic_string<char, AZStd::char_traits<char>, AZ::OSStdAllocator>::format("%s\\server.log", s_gameLiftSettings.m_logPath).c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH);
        m_logFile.Close();

        BusConnect();
    }
    virtual ~TraceDrillerHook()
    {
        BusDisconnect();
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy(); // used by the bus

        m_logFile.Close();
    }
    virtual bool OnPrintf(const char* window, const char* message)
    {
        m_logFile.ReOpen(AZ::IO::SystemFile::SF_OPEN_APPEND);

        m_logFile.Write(window, strlen(window));
        m_logFile.Write(": ", 2);
        m_logFile.Write(message, strlen(message));

        m_logFile.Close();

        return true;
    }

    AZ::IO::SystemFile m_logFile;
};

class GameLiftTest 
    : public GridMate::SessionEventBus::Handler
    , public GridMate::GameLiftServerServiceEventsBus::Handler
{
public:
    GameLiftTest()
        : m_gridMate(nullptr)
        , m_service(nullptr)
    {
    }

    ~GameLiftTest()
    {
        AZ_Assert(!m_service, "System was not shutdown properly!");
        AZ_Assert(!m_gridMate, "System was not shutdown properly!");
    }

    void Init(const TestSettings& settings)
    {
        // Initializing system allocators
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        // Creating Gridmate instance
        GridMate::GridMateDesc gridmateDesc;
        m_gridMate = GridMate::GridMateCreate(gridmateDesc);

        // Initializing GridMate multiplayer service allocator
        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

        // Subscribing for session events
        GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);
        GridMate::GameLiftServerServiceEventsBus::Handler::BusConnect(m_gridMate);

        // Starting GameLift multiplayer service
        GridMate::GameLiftServerServiceDesc serviceDesc;
        serviceDesc.m_port = settings.m_serverPort;
        if (settings.m_logPath)
        {
            serviceDesc.m_logPaths.push_back(settings.m_logPath);
        }

        m_service = GridMate::StartGridMateService<GridMate::GameLiftServerService>(m_gridMate, serviceDesc);
    }

    void Shutdown()
    {
        if (m_gridMate)
        {
            GridMate::GameLiftServerServiceEventsBus::Handler::BusDisconnect();
            GridMate::SessionEventBus::Handler::BusDisconnect();
            GridMateDestroy(m_gridMate);

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

            m_service = nullptr;
            m_gridMate = nullptr;
        }
    }

protected:
    GridMate::IGridMate* m_gridMate;
    GridMate::GameLiftServerService* m_service;
};

/*
* This is helper class that runs host session on GameLift instance. Just runs infinite loop to accept incoming peers.
* Client side test checks that GameLift parameter "param1" exists and accepts value "value12",
* So you need to make sure that fleet you created supports this parameter.
*/
class GameLiftSampleHost
    : public GameLiftTest
{
public:
    void run()
    {
        m_session = nullptr;

        Init(s_gameLiftSettings);

        while (true)
        {
            m_gridMate->Update();
            if (m_session)
            {
                m_session->GetReplicaMgr()->Unmarshal();
                m_session->GetReplicaMgr()->UpdateFromReplicas();
                m_session->GetReplicaMgr()->UpdateReplicas();
                m_session->GetReplicaMgr()->Marshal();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(s_tickDelayMs));
        }

        Shutdown();
    }

    void OnGameLiftGameSessionStarted(GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::GameSession& gameSession) override
    {
        for (GridMate::GridSession* session : service->GetSessions()) // leaving previously created sessions
        {
            session->Leave(false);
        }

        GridMate::GameLiftSessionParams sp;
        sp.m_topology = GridMate::ST_CLIENT_SERVER;
        sp.m_numPublicSlots = 16;
        sp.m_flags = 0;
        sp.m_gameSession = &gameSession;

        GridMate::CarrierDesc carrierDesc;
        carrierDesc.m_enableDisconnectDetection = true;
        carrierDesc.m_connectionTimeoutMS = 10000;
        carrierDesc.m_threadUpdateTimeMS = 30;

        // GameLift doesn't seem to be reporting the correct port to host on.
        //carrierDesc.m_port = gameSession.GetPort();
        carrierDesc.m_port = s_gameLiftSettings.m_serverPort;

        carrierDesc.m_driverIsFullPackets = false;
        carrierDesc.m_driverIsCrossPlatform = true;

        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_ID_RESULT(session, m_gridMate, GridMate::GameLiftServerServiceBus, HostSession, sp, carrierDesc);
        if (session)
        {
            AZ_Printf("GameLift", "Started session hosting on port %d.\n", carrierDesc.m_port);
        }
        else
        {
            AZ_Printf("GameLift", "Error creating host session.\n");
        }
    }

    GridMate::GridSession* m_session;
};

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        AZ_Printf("GameLift", "Supported settings:\n"
            "  serverPort:<int>       - The port the server will be listening on\n"
            );

        ::exit(-1);
    }

    for (int i = 1; i < argc; ++i)
    {
        char* pos = strchr(argv[i], ':');
        if (!pos)
        {
            AZ_Printf("GameLift", "Invalid argument format: %s. Expected 'key:value'.", argv[i]);
            continue;
        }

        *pos = '\0';
        const char* paramName = argv[i];
        const char* paramVal = pos + 1;

        if (!azstricmp(paramName, "serverPort"))
        {
            s_gameLiftSettings.m_serverPort = atoi(paramVal);
        }
        else if (!azstricmp(paramName, "logpath"))
        {
            s_gameLiftSettings.m_logPath = paramVal;
        }
        else
        {
            AZ_Printf("GameLift", "Unsupported parameter: %s", argv[i]);
        }
    }

    // Attach the hook after we read the command line arguments so we can write the log to the right place.
    TraceDrillerHook logHook;

    GameLiftSampleHost host;
    host.run();

    return 0;
}
