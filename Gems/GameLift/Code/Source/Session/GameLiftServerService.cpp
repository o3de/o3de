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

#if defined(BUILD_GAMELIFT_SERVER)

#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/Memory.h>

#include <GameLift/Session/GameLiftServerSDKWrapper.h>
#include <GameLift/Session/GameLiftServerService.h>
#include <GameLift/Session/GameLiftServerSession.h>


namespace GridMate
{
    GameLiftServerService::GameLiftServerService(const GameLiftServerServiceDesc& desc)
        : SessionService(desc)
        , m_serviceDesc(desc)
        , m_serverStatus(GameLift_NotInited)
        , m_serverInitOutcome(nullptr)
    {
        m_gameLiftServerSDKWrapper = AZStd::make_shared<GridMate::GameLiftServerSDKWrapper>();
    }

    GameLiftServerService::~GameLiftServerService()
    {
        m_gameLiftServerSDKWrapper.reset();
    }

    void GameLiftServerService::OnServiceRegistered(IGridMate* gridMate)
    {
        SessionService::OnServiceRegistered(gridMate);

        GameLiftServerSession::RegisterReplicaChunks();

        Internal::GameLiftServerSystemEventsBus::Handler::BusConnect();

        if (!StartGameLiftServer())
        {
            EBUS_EVENT_ID(m_gridMate, GameLiftServerServiceEventsBus, OnGameLiftSessionServiceFailed, this);
        }

        GameLiftServerServiceBus::Handler::BusConnect(gridMate);
    }

    void GameLiftServerService::OnServiceUnregistered(IGridMate* gridMate)
    {
        GameLiftServerServiceBus::Handler::BusDisconnect();

        Internal::GameLiftServerSystemEventsBus::Handler::BusDisconnect();

        Internal::GameLiftServerSystemEventsBus::ClearQueuedEvents();

        if (m_serverStatus == GameLift_Ready || m_serverStatus == GameLift_Terminated)
        {
            GetGameLiftServerSDKWrapper().lock()->ProcessEnding();
            GetGameLiftServerSDKWrapper().lock()->Destroy();
            m_serverStatus = GameLift_NotInited;
        }

        delete m_serverInitOutcome;

        SessionService::OnServiceUnregistered(gridMate);
    }

    bool GameLiftServerService::StartGameLiftServer()
    {
        if (m_serverStatus == GameLift_NotInited)
        {
            Aws::GameLift::Server::InitSDKOutcome initOutcome = GetGameLiftServerSDKWrapper().lock()->InitSDK();
            if (initOutcome.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "InitSDK succeeded.\n");
                AZ_Warning("GameLift", m_serviceDesc.m_port != 0, "Server will be listening on ephemeral port");

                std::vector<std::string> logPaths;
                for (const string& path : m_serviceDesc.m_logPaths)
                {
                    logPaths.push_back(path.c_str());
                }

                Aws::GameLift::Server::ProcessParameters processParams(
                    /*
                    * onStartGameSession
                    * Invoked when we push a GameSession to the server
                    */
                    [this](const Aws::GameLift::Server::Model::GameSession& gameSession) {
                        AZ_TracePrintf("GameLift", "On Activate...\n");
                        // This callback will be called on GameLift thread
                        EBUS_QUEUE_EVENT(Internal::GameLiftServerSystemEventsBus, OnGameLiftGameSessionStarted, gameSession);
                    },
                    /*
                    * onUpdateGameSession
                    * Invoked when the game session is updated after backfill
                    */
                    [this](const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession) {
                        AZ_TracePrintf("GameLift", "On Update Game Session...\n");
                        EBUS_QUEUE_EVENT(Internal::GameLiftServerSystemEventsBus, OnGameLiftGameSessionUpdated, updateGameSession);
                    },
                    /*
                    * onProcessTerminate
                    * Invoked when GameLift wants to force kill the server
                    */
                    [this]() {
                        AZ_TracePrintf("GameLift", "On Terminate invoked\n");
                        EBUS_QUEUE_EVENT(Internal::GameLiftServerSystemEventsBus, OnGameLiftServerWillTerminate);
                    },

                    /*
                    * onHealthCheck
                    * Invoked every minute to check on health. This callback must return a boolean. 
                    */
                    []() {
                        return true;
                    },

                    /*
                    * port
                    * The port the server will be listening on
                    */
                    m_serviceDesc.m_port,

                    /*
                    * logParameters
                    * A vector of log paths the servers will write to (and uploaded)
                    */
                    Aws::GameLift::Server::LogParameters(logPaths)
                );
                m_serverInitOutcome = new Aws::GameLift::GenericOutcomeCallable(
                    GetGameLiftServerSDKWrapper().lock()->ProcessReadyAsync(processParams));
            }
            else
            {
                AZ_TracePrintf("GameLift", "InitSDK failed.\n");
                m_serverStatus = GameLift_Failed;
            }
        }

        return m_serverStatus != GameLift_Failed;
    }

    bool GameLiftServerService::IsReady() const
    {
        return m_serverStatus == GameLift_Ready;
    }

    void GameLiftServerService::OnGameLiftGameSessionStarted(const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        AZ_TracePrintf("GameLift", "Dispatching OnGameLiftGameSessionStarted...\n");
        EBUS_EVENT_ID(m_gridMate, GameLiftServerServiceEventsBus, OnGameLiftGameSessionStarted, this, gameSession);
    }

    void GameLiftServerService::OnGameLiftGameSessionUpdated(const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession)
    {
        AZ_TracePrintf("GameLift", "Dispatching OnGameLiftGameSessionUpdated...\n");
        UpdateGameSession(updateGameSession);
        EBUS_EVENT_ID(m_gridMate, GameLiftServerServiceEventsBus, OnGameLiftGameSessionUpdated, this, updateGameSession);
    }

    void GameLiftServerService::OnGameLiftServerWillTerminate()
    {
        Internal::GameLiftServerSystemEventsBus::Handler::BusDisconnect();

        Internal::GameLiftServerSystemEventsBus::ClearQueuedEvents(); // already terminating, don't need any other events
        m_serverStatus = GameLift_Terminated;
        EBUS_EVENT_ID(m_gridMate, GameLiftServerServiceEventsBus, OnGameLiftServerWillTerminate, this);
    }

    void GameLiftServerService::Update()
    {
        Internal::GameLiftServerSystemEventsBus::ExecuteQueuedEvents();

        if (m_serverInitOutcome
            && m_serverInitOutcome->valid()
            && m_serverInitOutcome->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_serverInitOutcome->get();
            if (result.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "Initialized GameLift server successfully.\n");
                m_serverStatus = GameLift_Ready;

                if (IsReady())
                {
                    EBUS_EVENT_ID(m_gridMate, GameLiftServerServiceEventsBus, OnGameLiftSessionServiceReady, this);
                    EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionServiceReady);
                    EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionServiceReady);
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "Failed to initialize GameLift server: %s, %s\n", result.GetError().GetErrorName().c_str(), result.GetError().GetErrorMessage().c_str());
                m_serverStatus = GameLift_Failed;
                // defer the notification so Gridmate doesn't destroy this service while updating
                AZ::TickBus::QueueFunction([this]()
                {
                    GameLiftServerServiceEventsBus::Event(m_gridMate, &GameLiftServerServiceEventsBus::Events::OnGameLiftSessionServiceFailed, this);
                });
            }
        }

        SessionService::Update();
    }

    GameLiftServerSession* GameLiftServerService::QueryGameLiftSession(const GridSession* session)
    {
        for (GridSession* s : m_sessions)
        {
            if (s == session)
            {
                return static_cast<GameLiftServerSession*>(s);
            }
        }

        return nullptr;
    }

    GridSession* GameLiftServerService::HostSession(const GameLiftSessionParams& params, const CarrierDesc& carrierDesc)
    {
        AZ_TracePrintf("GameLift", "GameLiftSessionService::HostSession.\n");
        if (m_serverStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Server API is not initialized.\n");
            return nullptr;
        }

        GameLiftServerSession* session = aznew GameLiftServerSession(this);
        if (!session->Initialize(params, carrierDesc))
        {
            AZ_TracePrintf("GameLift", "GameLiftSessionService::HostSession. Could not initialize the session.\n");
            delete session;
            return nullptr;
        }
        AZ_TracePrintf("GameLift", "GameLiftSessionService::HostSession. Completed.\n");
        return session;
    }

    void GameLiftServerService::ShutdownSession(const GridSession* gridSession)
    {
        GameLiftServerSession* session = FindGameLiftServerSession(gridSession->GetId());
        if (session)
        {
            // Shutdown call removes the game session from server service.
            session->Shutdown();
            delete session;
        }
        else
        {
            AZ_TracePrintf("GameLift", "GameSession Failed to Shutdown. No GameLiftServerSession found for :%s", gridSession->GetId().c_str());
        }
    }

    bool GameLiftServerService::StartMatchmakingBackfill(const GridSession* gridSession, AZStd::string& matchmakingTicketId, bool checkForAutoBackfill)
    {
        GameLiftServerSession* session = FindGameLiftServerSession(gridSession->GetId());
        if (session)
        {
            return session->StartMatchmakingBackfill(matchmakingTicketId, checkForAutoBackfill);
        }
        else
        {
            AZ_TracePrintf("GameLift", "GameSession Failed to start backfill. No GameLiftServerSession found for :%s", gridSession->GetId().c_str());
            return false;
        }
    }

    bool GameLiftServerService::StopMatchmakingBackfill(const GridSession* gridSession, const AZStd::string& matchmakingTicketId)
    {
        GameLiftServerSession* session = FindGameLiftServerSession(gridSession->GetId());
        if (session)
        {
            return session->StopMatchmakingBackfill(matchmakingTicketId);
        }
        else
        {
            AZ_TracePrintf("GameLift", "GameSession Failed to stop backfill. No GameLiftServerSession found for :%s", gridSession->GetId().c_str());
            return false;
        }
    }

    bool GameLiftServerService::UpdateGameSession(const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession)
    {
        AZStd::string gameSessionId = updateGameSession.GetGameSession().GetGameSessionId().c_str();
        GameLiftServerSession* session = FindGameLiftServerSession(gameSessionId);
        if (session)
        {            
            return session->GameSessionUpdated(updateGameSession);
        }
        else
        {
            AZ_TracePrintf("GameLift", "GameSession Failed to update. No GameLiftServerSession found for :%s", gameSessionId.c_str());
            return false;
        }
    }

    AZStd::weak_ptr<GameLiftServerSDKWrapper> GameLiftServerService::GetGameLiftServerSDKWrapper()
    {
        return m_gameLiftServerSDKWrapper;
    }

    GameLiftServerSession* GameLiftServerService::FindGameLiftServerSession(const AZStd::string& id)
    {
        GridSession* gridSession = nullptr;
        for (GridSession* session : m_sessions)
        {
            if (id.compare(session->GetId()) == 0)
            {
                gridSession = session;
                break;
            }
        }
        return static_cast<GameLiftServerSession*>(gridSession);
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER
