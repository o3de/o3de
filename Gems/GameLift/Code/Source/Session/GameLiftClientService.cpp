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

#if defined(BUILD_GAMELIFT_CLIENT)

#include <AzCore/Component/TickBus.h>
#include <AzFramework/AzFramework_Traits_Platform.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftClientSession.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftSessionRequest.h>
#include <GameLift/Session/GameLiftGameSessionPlacementRequest.h>
#include <GameLift/Session/GameLiftMatchmaking.h>
#include <GameLift/Session/GameLiftRequestInterface.h>
#include <AzCore/IO/FileIO.h>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/ListBuildsRequest.h>

namespace
{
    //This is used when a playerId is not specified when initializing GameLiftSDK with developer credentials.
    const char* DEFAULT_PLAYER_ID = "AnonymousPlayerId";
}

namespace GridMate
{
    namespace Platform
    {
        void ResolveCaCertFilePath(Aws::String& caFile);
    }

    GameLiftClientService::GameLiftClientService(const GameLiftClientServiceDesc& desc)
        : SessionService(desc)
        , m_serviceDesc(desc)
        , m_clientStatus(GameLift_NotInited)
    {
    }

    GameLiftClientService::~GameLiftClientService()
    {

    }

    bool GameLiftClientService::IsReady() const
    {
        return m_clientStatus == GameLift_Ready;
    }

    AZStd::shared_ptr<Aws::GameLift::GameLiftClient> GameLiftClientService::GetClient() const
    {
        return m_clientSharedPtr;
    }

    Aws::String GameLiftClientService::GetPlayerId() const
    {
        return m_serviceDesc.m_playerId.c_str();
    }

    void GameLiftClientService::OnServiceRegistered(IGridMate* gridMate)
    {
        SessionService::OnServiceRegistered(gridMate);

        GameLiftClientSession::RegisterReplicaChunks();

        if (!StartGameLiftClient())
        {
            EBUS_EVENT_ID(m_gridMate, GameLiftClientServiceEventsBus, OnGameLiftSessionServiceFailed, this, "GameLift client failed to start");
        }

        GameLiftClientServiceBus::Handler::BusConnect(gridMate);
    }

    void GameLiftClientService::OnServiceUnregistered(IGridMate* gridMate)
    {
        GameLiftClientServiceBus::Handler::BusDisconnect();

        if (m_clientStatus == GameLift_Ready)
        {
            m_clientSharedPtr.reset();
            m_clientStatus = GameLift_NotInited;
        }

        SessionService::OnServiceUnregistered(gridMate);
    }

    void GameLiftClientService::Update()
    {
        if (m_listBuildsOutcomeCallable.valid() && m_listBuildsOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto outcome = m_listBuildsOutcomeCallable.get();
            if (outcome.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "Initialized GameLift client successfully.\n");
                m_clientStatus = GameLift_Ready;

                EBUS_EVENT_ID(m_gridMate, GameLiftClientServiceEventsBus, OnGameLiftSessionServiceReady, this);
                EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionServiceReady);
                EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionServiceReady);
            }
            else
            {
                auto errorMessage = outcome.GetError().GetMessage();
                AZ_TracePrintf("GameLift", "Failed to initialize GameLift client: %s\n", errorMessage.c_str());

                m_clientStatus = GameLift_Failed;

                // defer the notification so Gridmate doesn't destroy this service while updating
                AZ::TickBus::QueueFunction([this, errorMessage]()
                {
                    GameLiftClientServiceEventsBus::Event(m_gridMate, &GameLiftClientServiceEventsBus::Events::OnGameLiftSessionServiceFailed, this, errorMessage.c_str());
                });
            }
        }

        SessionService::Update();
    }

    GridSession* GameLiftClientService::JoinSessionBySearchInfo(const GameLiftSearchInfo& searchInfo, const CarrierDesc& carrierDesc)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }

        GameLiftClientSession* session = aznew GameLiftClientSession(this);
        if (!session->Initialize(searchInfo, JoinParams(), carrierDesc))
        {
            delete session;
            return nullptr;
        }

        return session;
    }

    GridSearch* GameLiftClientService::RequestSession(const GameLiftSessionRequestParams& params)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }

        AZStd::shared_ptr<GameLiftRequestInterfaceContext> context = AZStd::make_shared<GameLiftRequestInterfaceContext>();
        context->m_gameLiftClient = m_clientSharedPtr;
        context->m_playerId = m_serviceDesc.m_playerId.c_str();

        // If queue name is set use queues to create a GameLift session. else use fleetId.
        if (!params.m_queueName.empty())
        {
            context->m_requestParams = params;
            GameLiftGameSessionPlacementRequest* request = aznew GameLiftGameSessionPlacementRequest(this, context);
            if (!request->Initialize())
            {
                delete request;
                return nullptr;
            }
            return request;
        }
        else
        {
            context->m_requestParams = params;
            GameLiftSessionRequest* request = aznew GameLiftSessionRequest(this, context);
            if (!request->Initialize())
            {
                delete request;
                return nullptr;
            }
            return request;
        }
    }

    GridSearch* GameLiftClientService::StartMatchmaking(const AZStd::string& matchmakingConfigName)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }

        AZStd::shared_ptr<GameLiftRequestInterfaceContext> context = AZStd::make_shared<GameLiftRequestInterfaceContext>();
        context->m_gameLiftClient = m_clientSharedPtr;
        context->m_playerId = m_serviceDesc.m_playerId.c_str();

        GameLiftMatchmaking* request = aznew GameLiftMatchmaking(this, context, Aws::String(matchmakingConfigName.c_str()));
        if (!request->Initialize())
        {
            delete request;
            return nullptr;
        }
        return request;
    }

    GameLiftSearch* GameLiftClientService::StartSearch(const GameLiftSearchParams& params)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }
        AZStd::shared_ptr<GameLiftRequestInterfaceContext> context = AZStd::make_shared<GameLiftRequestInterfaceContext>();
        context->m_gameLiftClient = m_clientSharedPtr;
        context->m_searchParams = params;
        GameLiftSearch* search = aznew GameLiftSearch(this, context);
        if (!search->Initialize())
        {
            delete search;
            return nullptr;
        }

        return search;
    }

    GameLiftClientSession* GameLiftClientService::QueryGameLiftSession(const GridSession* session)
    {
        for (GridSession* s : m_sessions)
        {
            if (s == session)
            {
                return static_cast<GameLiftClientSession*>(s);
            }
        }

        return nullptr;
    }

    GameLiftSearch* GameLiftClientService::QueryGameLiftSearch(const GridSearch* search)
    {
        for (GridSearch* s : m_activeSearches)
        {
            if (s == search)
            {
                return static_cast<GameLiftSearch*>(s);
            }
        }

        for (GridSearch* s : m_completedSearches)
        {
            if (s == search)
            {
                return static_cast<GameLiftSearch*>(s);
            }
        }

        return nullptr;
    }

    bool GameLiftClientService::StartGameLiftClient()
    {
        if (m_clientStatus == GameLift_NotInited)
        {
            if (!ValidateAWSCredentials())
            {
                m_clientStatus = GameLift_Failed;
            }
            else
            {
                m_clientStatus = GameLift_Initing;
                CreateSharedAWSGameLiftClient();
                Aws::GameLift::Model::ListBuildsRequest request;
                m_listBuildsOutcomeCallable = m_clientSharedPtr->ListBuildsCallable(request);
            }
        }

        return m_clientStatus != GameLift_Failed;
    }


    void GameLiftClientService::CreateSharedAWSGameLiftClient()
    {
        Aws::Client::ClientConfiguration config;
        config.enableTcpKeepAlive = AZ_TRAIT_AZFRAMEWORK_AWS_ENABLE_TCP_KEEP_ALIVE_SUPPORTED;
        config.region = m_serviceDesc.m_region.c_str();
        config.endpointOverride = m_serviceDesc.m_endpoint.c_str();

        if (m_serviceDesc.m_useGameLiftLocalServer)
        {
            config.verifySSL = false;
            config.scheme = Aws::Http::Scheme::HTTP;
        }
        else
        {
            config.verifySSL = true;
            config.scheme = Aws::Http::Scheme::HTTPS;

            Platform::ResolveCaCertFilePath(config.caFile);
        }

        Aws::String accessKey(m_serviceDesc.m_accessKey.c_str());
        Aws::String secretKey(m_serviceDesc.m_secretKey.c_str());
        Aws::Auth::AWSCredentials cred = Aws::Auth::AWSCredentials(accessKey, secretKey);

        m_clientSharedPtr = AZStd::make_shared<Aws::GameLift::GameLiftClient>(cred, config);
    }

    bool GameLiftClientService::ValidateAWSCredentials()
    {
        if (m_serviceDesc.m_accessKey.empty() || m_serviceDesc.m_secretKey.empty())
        {
            AZ_TracePrintf("GameLift", "Initialize failed. Cannot use GameLift without access and secret key.\n");
            return false;
        }

        return true;
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
