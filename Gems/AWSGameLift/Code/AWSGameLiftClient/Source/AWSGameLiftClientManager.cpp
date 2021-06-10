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

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Session/SessionConfig.h>

#include <AWSCoreBus.h>
#include <Credential/AWSCredentialBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <AWSGameLiftClientManager.h>
#include <Activity/AWSGameLiftCreateSessionActivity.h>
#include <Activity/AWSGameLiftJoinSessionActivity.h>
#include <Activity/AWSGameLiftSearchSessionsActivity.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace AWSGameLift
{
#if defined(AWSGAMELIFT_DEV)
    AZ_CVAR(AZ::CVarFixedString, cl_gameliftLocalEndpoint, "", nullptr, AZ::ConsoleFunctorFlags::Null, "The local endpoint to test with GameLiftLocal SDK.");
#endif

    AWSGameLiftClientManager::AWSGameLiftClientManager()
    {
        m_gameliftClient.reset();
    }

    void AWSGameLiftClientManager::ActivateManager()
    {
        AZ::Interface<IAWSGameLiftRequests>::Register(this);
        AWSGameLiftRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::ISessionAsyncRequests>::Register(this);
        AWSGameLiftSessionAsyncRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::ISessionRequests>::Register(this);
        AWSGameLiftSessionRequestBus::Handler::BusConnect();
    }

    void AWSGameLiftClientManager::DeactivateManager()
    {
        AWSGameLiftSessionRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::ISessionRequests>::Unregister(this);

        AWSGameLiftSessionAsyncRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::ISessionAsyncRequests>::Unregister(this);

        AWSGameLiftRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSGameLiftRequests>::Unregister(this);
    }

    bool AWSGameLiftClientManager::ConfigureGameLiftClient(const AZStd::string& region)
    {
        m_gameliftClient.reset();

        Aws::Client::ClientConfiguration clientConfig;
        // Set up client endpoint or region
        AZStd::string localEndpoint = "";
#if defined(AWSGAMELIFT_DEV)
        localEndpoint = static_cast<AZ::CVarFixedString>(cl_gameliftLocalEndpoint);
#endif
        if (!localEndpoint.empty())
        {
            // The attribute needs to override to interact with GameLiftLocal
            clientConfig.endpointOverride = localEndpoint.c_str();
        }
        else if (!region.empty())
        {
            clientConfig.region = region.c_str();
        }
        else
        {
            AZStd::string clientRegion;
            AWSCore::AWSResourceMappingRequestBus::BroadcastResult(clientRegion, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);
            if (clientRegion.empty())
            {
                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientRegionMissingErrorMessage);
                return false;
            }
            clientConfig.region = clientRegion.c_str();
        }

        // Fetch AWS credential for client
        AWSCore::AWSCredentialResult credentialResult;
        AWSCore::AWSCredentialRequestBus::BroadcastResult(credentialResult, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
        if (!localEndpoint.empty())
        {
            credentialResult.result = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
        }
        else if (!credentialResult.result)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientCredentialMissingErrorMessage);
            return false;
        }
        m_gameliftClient = AZStd::make_shared<Aws::GameLift::GameLiftClient>(credentialResult.result, clientConfig);
        return true;
    }

    AZStd::string AWSGameLiftClientManager::CreatePlayerId(bool includeBrackets, bool includeDashes)
    {
        return AZ::Uuid::CreateRandom().ToString<AZStd::string>(includeBrackets, includeDashes);
    }

    AZStd::string AWSGameLiftClientManager::CreateSession(const AzFramework::CreateSessionRequest& createSessionRequest)
    {
        // Keep the GameLift client object when another thread tries to reset the client during this request.
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameLiftClient = m_gameliftClient;

        AZStd::string result = "";
        if (!gameLiftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else if (CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
        {
            AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting CreateGameSession against Amazon GameLift service...");

            auto& gameliftCreateSessionRequest = static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);
            result = CreateSessionActivity::CreateSession(*gameLiftClient, gameliftCreateSessionRequest);
        }

        return result;
    }

    void AWSGameLiftClientManager::CreateSessionAsync(const AzFramework::CreateSessionRequest& createSessionRequest)
    {
        if (!CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
        {
            AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, "");
            return;
        }

        const AWSGameLiftCreateSessionRequest& gameliftCreateSessionRequest =
            static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* createSessionJob = AZ::CreateJobFunction(
            [this, gameliftCreateSessionRequest]()
            {
                // Keep the GameLift client object when another thread tries to reset the client during this request.
                AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameClient = m_gameliftClient;

                AZStd::string result;
                if (!gameClient)
                {
                    AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
                }
                else
                {
                    AZ_TracePrintf(
                        AWSGameLiftClientManagerName, "Requesting CreateGameSession against Amazon GameLift service asynchronously...");

                    result = CreateSessionActivity::CreateSession(*gameClient, gameliftCreateSessionRequest);
                }

                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, result);
            },
            true, jobContext);

        createSessionJob->Start();
    }

    bool AWSGameLiftClientManager::JoinSession(const AzFramework::JoinSessionRequest& joinSessionRequest)
    {
        // Keep the GameLift client object when another thread tries to reset the client during this request.
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameLiftClient = m_gameliftClient;

        bool result = false;
        if (!gameLiftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else if (JoinSessionActivity::ValidateJoinSessionRequest(joinSessionRequest))
        {
            AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting CreatePlayerSession call against Amazon GameLift service...");

            auto& gameliftJoinSessionRequest = static_cast<const AWSGameLiftJoinSessionRequest&>(joinSessionRequest);
            auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(*gameLiftClient, gameliftJoinSessionRequest);

            AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting player to connect to game session...");

            result = JoinSessionActivity::RequestPlayerJoinSession(
                createPlayerSessionOutcome);
        }

        return result;
    }

    void AWSGameLiftClientManager::JoinSessionAsync(const AzFramework::JoinSessionRequest& joinSessionRequest)
    {
        if (!JoinSessionActivity::ValidateJoinSessionRequest(joinSessionRequest))
        {
            AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, false);
            return;
        }

        const AWSGameLiftJoinSessionRequest& gameliftJoinSessionRequest =
            static_cast<const AWSGameLiftJoinSessionRequest&>(joinSessionRequest);

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* joinSessionJob = AZ::CreateJobFunction(
            [this, gameliftJoinSessionRequest]()
            {
                // Keep the GameLift client object when another thread tries to reset the client during this request.
                AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameClient = m_gameliftClient;
                bool result = false;
                if (!gameClient)
                {
                    AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
                }
                else
                {
                    AZ_TracePrintf(AWSGameLiftClientManagerName,
                        "Requesting CreatePlayerSession call against Amazon GameLift service asynchronously...");

                    auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(*gameClient, gameliftJoinSessionRequest);

                    AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting player to connect to game session asynchronously...");

                    result = JoinSessionActivity::RequestPlayerJoinSession(createPlayerSessionOutcome);
                }
                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, result);
            },
            true, jobContext);

        joinSessionJob->Start();
    }

    void AWSGameLiftClientManager::LeaveSession()
    {
        // TODO: Add implementation for leave session
    }

    void AWSGameLiftClientManager::LeaveSessionAsync()
    {
        // TODO: Add implementation for leave session
    }

    AzFramework::SearchSessionsResponse AWSGameLiftClientManager::SearchSessions(
        const AzFramework::SearchSessionsRequest& searchSessionsRequest) const
    {
        // Keep the GameLift client object when another thread tries to reset the client during this request.
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameLiftClient = m_gameliftClient;

        AzFramework::SearchSessionsResponse response;
        if (!gameLiftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else if (SearchSessionsActivity::ValidateSearchSessionsRequest(searchSessionsRequest))
        {
            AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting SearchGameSessions against Amazon GameLift service...");

            const AWSGameLiftSearchSessionsRequest& gameliftSearchSessionsRequest =
                static_cast<const AWSGameLiftSearchSessionsRequest&>(searchSessionsRequest);
            response = SearchSessionsActivity::SearchSessions(*gameLiftClient, gameliftSearchSessionsRequest);
        }

        return response;
    }

    void AWSGameLiftClientManager::SearchSessionsAsync(const AzFramework::SearchSessionsRequest& searchSessionsRequest) const
    {
        if (!SearchSessionsActivity::ValidateSearchSessionsRequest(searchSessionsRequest))
        {
            AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                &AzFramework::SessionAsyncRequestNotifications::OnSearchSessionsAsyncComplete, AzFramework::SearchSessionsResponse());
            return;
        }
        const AWSGameLiftSearchSessionsRequest& gameliftSearchSessionsRequest =
            static_cast<const AWSGameLiftSearchSessionsRequest&>(searchSessionsRequest);

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* searchSessionsJob = AZ::CreateJobFunction(
            [this, gameliftSearchSessionsRequest]()
            {
                // Keep the GameLift client object when another thread tries to reset the client during this request.
                AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameClient = m_gameliftClient;

                AzFramework::SearchSessionsResponse response;
                if (!gameClient)
                {
                    AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
                }
                else
                {
                    AZ_TracePrintf(
                        AWSGameLiftClientManagerName, "Requesting SearchGameSessions against Amazon GameLift service asynchronously...");

                    response = SearchSessionsActivity::SearchSessions(*gameClient, gameliftSearchSessionsRequest);
                }
                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnSearchSessionsAsyncComplete, response);
            },
            true, jobContext);

        searchSessionsJob->Start(); 
    }

    void AWSGameLiftClientManager::SetGameLiftClient(AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient)
    {
        m_gameliftClient.swap(gameliftClient);
    }
} // namespace AWSGameLift
