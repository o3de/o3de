/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Session/SessionConfig.h>

#include <AWSCoreBus.h>
#include <Credential/AWSCredentialBus.h>
#include <Framework/AWSApiJobConfig.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftSessionConstants.h>
#include <Activity/AWSGameLiftAcceptMatchActivity.h>
#include <Activity/AWSGameLiftCreateSessionActivity.h>
#include <Activity/AWSGameLiftCreateSessionOnQueueActivity.h>
#include <Activity/AWSGameLiftJoinSessionActivity.h>
#include <Activity/AWSGameLiftLeaveSessionActivity.h>
#include <Activity/AWSGameLiftSearchSessionsActivity.h>
#include <Request/IAWSGameLiftInternalRequests.h>
#include <Activity/AWSGameLiftStartMatchmakingActivity.h>
#include <Activity/AWSGameLiftStopMatchmakingActivity.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace AWSGameLift
{
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
    AZ_CVAR(AZ::CVarFixedString, cl_gameliftLocalEndpoint, "", nullptr, AZ::ConsoleFunctorFlags::Null, "The local endpoint to test with GameLiftLocal SDK.");
#endif

    void AWSGameLiftClientManager::ActivateManager()
    {
        AZ::Interface<IAWSGameLiftRequests>::Register(this);
        AWSGameLiftRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::ISessionAsyncRequests>::Register(this);
        AWSGameLiftSessionAsyncRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::ISessionRequests>::Register(this);
        AWSGameLiftSessionRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::IMatchmakingAsyncRequests>::Register(this);
        AWSGameLiftMatchmakingAsyncRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::IMatchmakingRequests>::Register(this);
        AWSGameLiftMatchmakingRequestBus::Handler::BusConnect();
    }

    void AWSGameLiftClientManager::DeactivateManager()
    {
        AWSGameLiftMatchmakingRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::IMatchmakingRequests>::Unregister(this);

        AWSGameLiftMatchmakingAsyncRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::IMatchmakingAsyncRequests>::Unregister(this);

        AWSGameLiftSessionRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::ISessionRequests>::Unregister(this);

        AWSGameLiftSessionAsyncRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::ISessionAsyncRequests>::Unregister(this);

        AWSGameLiftRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSGameLiftRequests>::Unregister(this);
    }

    bool AWSGameLiftClientManager::ConfigureGameLiftClient(const AZStd::string& region)
    {
        AZ::Interface<IAWSGameLiftInternalRequests>::Get()->SetGameLiftClient(nullptr);

        Aws::Client::ClientConfiguration clientConfig;
        AWSCore::AwsApiJobConfig* defaultConfig = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(defaultConfig, &AWSCore::AWSCoreRequests::GetDefaultConfig);
        if (defaultConfig)
        {
            clientConfig = defaultConfig->GetClientConfiguration();
        }

        // Set up client endpoint or region
        AZStd::string localEndpoint = "";
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
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
        AZ::Interface<IAWSGameLiftInternalRequests>::Get()->SetGameLiftClient(
            AZStd::make_shared<Aws::GameLift::GameLiftClient>(credentialResult.result, clientConfig));
        return true;
    }

    AZStd::string AWSGameLiftClientManager::CreatePlayerId(bool includeBrackets, bool includeDashes)
    {
        return AZ::Uuid::CreateRandom().ToString<AZStd::string>(includeBrackets, includeDashes);
    }

    void AWSGameLiftClientManager::AcceptMatch(const AzFramework::AcceptMatchRequest& acceptMatchRequest)
    {
        if (AcceptMatchActivity::ValidateAcceptMatchRequest(acceptMatchRequest))
        {
            const AWSGameLiftAcceptMatchRequest& gameliftStartMatchmakingRequest =
                static_cast<const AWSGameLiftAcceptMatchRequest&>(acceptMatchRequest);
            AcceptMatchActivity::AcceptMatch(gameliftStartMatchmakingRequest);
        }
    }

    void AWSGameLiftClientManager::AcceptMatchAsync(const AzFramework::AcceptMatchRequest& acceptMatchRequest)
    {
        if (!AcceptMatchActivity::ValidateAcceptMatchRequest(acceptMatchRequest))
        {
            AzFramework::MatchmakingAsyncRequestNotificationBus::Broadcast(
                &AzFramework::MatchmakingAsyncRequestNotifications::OnAcceptMatchAsyncComplete);
            return;
        }

        const AWSGameLiftAcceptMatchRequest& gameliftStartMatchmakingRequest = static_cast<const AWSGameLiftAcceptMatchRequest&>(acceptMatchRequest);

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* acceptMatchJob = AZ::CreateJobFunction(
            [gameliftStartMatchmakingRequest]()
            {
                AcceptMatchActivity::AcceptMatch(gameliftStartMatchmakingRequest);

                AzFramework::MatchmakingAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::MatchmakingAsyncRequestNotifications::OnAcceptMatchAsyncComplete);
            },
            true, jobContext);

        acceptMatchJob->Start(); 
    }

    AZStd::string AWSGameLiftClientManager::CreateSession(const AzFramework::CreateSessionRequest& createSessionRequest)
    {
        AZStd::string result = "";
        if (CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
        {
            const AWSGameLiftCreateSessionRequest& gameliftCreateSessionRequest =
                static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);
            result = CreateSessionActivity::CreateSession(gameliftCreateSessionRequest);
        }
        else if (CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(createSessionRequest))
        {
            const AWSGameLiftCreateSessionOnQueueRequest& gameliftCreateSessionOnQueueRequest =
                static_cast<const AWSGameLiftCreateSessionOnQueueRequest&>(createSessionRequest);
            result = CreateSessionOnQueueActivity::CreateSessionOnQueue(gameliftCreateSessionOnQueueRequest);
        }
        else
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftCreateSessionRequestInvalidErrorMessage);
        }

        return result;
    }

    void AWSGameLiftClientManager::CreateSessionAsync(const AzFramework::CreateSessionRequest& createSessionRequest)
    {
        if (CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
        {
            const AWSGameLiftCreateSessionRequest& gameliftCreateSessionRequest =
                static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);

            AZ::JobContext* jobContext = nullptr;
            AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
            AZ::Job* createSessionJob = AZ::CreateJobFunction(
                [gameliftCreateSessionRequest]()
                {
                    AZStd::string result = CreateSessionActivity::CreateSession(gameliftCreateSessionRequest);

                    AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                        &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, result);
                },
                true, jobContext);
            createSessionJob->Start();
        }
        else if (CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(createSessionRequest))
        {
            const AWSGameLiftCreateSessionOnQueueRequest& gameliftCreateSessionOnQueueRequest =
                static_cast<const AWSGameLiftCreateSessionOnQueueRequest&>(createSessionRequest);

            AZ::JobContext* jobContext = nullptr;
            AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
            AZ::Job* createSessionOnQueueJob = AZ::CreateJobFunction(
                [gameliftCreateSessionOnQueueRequest]()
                {
                    AZStd::string result = CreateSessionOnQueueActivity::CreateSessionOnQueue(gameliftCreateSessionOnQueueRequest);

                    AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                        &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, result);
                },
                true, jobContext);
            createSessionOnQueueJob->Start();
        }
        else
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftCreateSessionRequestInvalidErrorMessage);
            AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, "");
        }
    }

    bool AWSGameLiftClientManager::JoinSession(const AzFramework::JoinSessionRequest& joinSessionRequest)
    {
        bool result = false;
        if (JoinSessionActivity::ValidateJoinSessionRequest(joinSessionRequest))
        {
            const AWSGameLiftJoinSessionRequest& gameliftJoinSessionRequest =
                static_cast<const AWSGameLiftJoinSessionRequest&>(joinSessionRequest);
            auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(gameliftJoinSessionRequest);
            result = JoinSessionActivity::RequestPlayerJoinSession(createPlayerSessionOutcome);
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
            [gameliftJoinSessionRequest]()
            {
                auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(gameliftJoinSessionRequest);
                bool result = JoinSessionActivity::RequestPlayerJoinSession(createPlayerSessionOutcome);

                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, result);
            },
            true, jobContext);

        joinSessionJob->Start();
    }

    void AWSGameLiftClientManager::LeaveSession()
    {
        AWSGameLift::LeaveSessionActivity::LeaveSession();
    }

    void AWSGameLiftClientManager::LeaveSessionAsync()
    {
        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* leaveSessionJob = AZ::CreateJobFunction(
            [this]()
            {
                LeaveSession();
                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnLeaveSessionAsyncComplete);
            },
            true, jobContext);

        leaveSessionJob->Start();
    }

    AzFramework::SearchSessionsResponse AWSGameLiftClientManager::SearchSessions(
        const AzFramework::SearchSessionsRequest& searchSessionsRequest) const
    {
        AzFramework::SearchSessionsResponse response;
        if (SearchSessionsActivity::ValidateSearchSessionsRequest(searchSessionsRequest))
        {
            const AWSGameLiftSearchSessionsRequest& gameliftSearchSessionsRequest =
                static_cast<const AWSGameLiftSearchSessionsRequest&>(searchSessionsRequest);
            response = SearchSessionsActivity::SearchSessions(gameliftSearchSessionsRequest);
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
            [gameliftSearchSessionsRequest]()
            {
                AzFramework::SearchSessionsResponse response = SearchSessionsActivity::SearchSessions(gameliftSearchSessionsRequest);

                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnSearchSessionsAsyncComplete, response);
            },
            true, jobContext);

        searchSessionsJob->Start(); 
    }

    AZStd::string AWSGameLiftClientManager::StartMatchmaking(const AzFramework::StartMatchmakingRequest& startMatchmakingRequest)
    {
        AZStd::string response;
        if (StartMatchmakingActivity::ValidateStartMatchmakingRequest(startMatchmakingRequest))
        {
            const AWSGameLiftStartMatchmakingRequest& gameliftStartMatchmakingRequest =
                static_cast<const AWSGameLiftStartMatchmakingRequest&>(startMatchmakingRequest);
            response = StartMatchmakingActivity::StartMatchmaking(gameliftStartMatchmakingRequest);
        }

        return response;
    }

    void AWSGameLiftClientManager::StartMatchmakingAsync(const AzFramework::StartMatchmakingRequest& startMatchmakingRequest)
    {
        if (!StartMatchmakingActivity::ValidateStartMatchmakingRequest(startMatchmakingRequest))
        {
            AzFramework::MatchmakingAsyncRequestNotificationBus::Broadcast(
                &AzFramework::MatchmakingAsyncRequestNotifications::OnStartMatchmakingAsyncComplete, AZStd::string{});
            return;
        }

        const AWSGameLiftStartMatchmakingRequest& gameliftStartMatchmakingRequest =
            static_cast<const AWSGameLiftStartMatchmakingRequest&>(startMatchmakingRequest);

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* startMatchmakingJob = AZ::CreateJobFunction(
            [gameliftStartMatchmakingRequest]()
            {
                AZStd::string response = StartMatchmakingActivity::StartMatchmaking(gameliftStartMatchmakingRequest);

                AzFramework::MatchmakingAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::MatchmakingAsyncRequestNotifications::OnStartMatchmakingAsyncComplete, response);
            },
            true, jobContext);

        startMatchmakingJob->Start(); 
    }

    void AWSGameLiftClientManager::StopMatchmaking(const AzFramework::StopMatchmakingRequest& stopMatchmakingRequest)
    {
        if (StopMatchmakingActivity::ValidateStopMatchmakingRequest(stopMatchmakingRequest))
        {
            const AWSGameLiftStopMatchmakingRequest& gameliftStopMatchmakingRequest =
                static_cast<const AWSGameLiftStopMatchmakingRequest&>(stopMatchmakingRequest);

            StopMatchmakingActivity::StopMatchmaking(gameliftStopMatchmakingRequest);
        }
    }

    void AWSGameLiftClientManager::StopMatchmakingAsync(const AzFramework::StopMatchmakingRequest& stopMatchmakingRequest)
    {
        if (!StopMatchmakingActivity::ValidateStopMatchmakingRequest(stopMatchmakingRequest))
        {
            AzFramework::MatchmakingAsyncRequestNotificationBus::Broadcast(
                &AzFramework::MatchmakingAsyncRequestNotifications::OnStopMatchmakingAsyncComplete);
            return;
        }

        const AWSGameLiftStopMatchmakingRequest& gameliftStopMatchmakingRequest =
            static_cast<const AWSGameLiftStopMatchmakingRequest&>(stopMatchmakingRequest);

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* stopMatchmakingJob = AZ::CreateJobFunction(
            [gameliftStopMatchmakingRequest]()
            {
                StopMatchmakingActivity::StopMatchmaking(gameliftStopMatchmakingRequest);

                AzFramework::MatchmakingAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::MatchmakingAsyncRequestNotifications::OnStopMatchmakingAsyncComplete);
            },
            true, jobContext);

        stopMatchmakingJob->Start(); 
    }
} // namespace AWSGameLift
