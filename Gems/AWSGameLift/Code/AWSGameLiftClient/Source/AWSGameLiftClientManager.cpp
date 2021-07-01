/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <AWSGameLiftClientManager.h>
#include <Activity/AWSGameLiftCreateSessionActivity.h>
#include <Activity/AWSGameLiftCreateSessionOnQueueActivity.h>
#include <Activity/AWSGameLiftJoinSessionActivity.h>
#include <Activity/AWSGameLiftLeaveSessionActivity.h>
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
        AZStd::string result = "";
        if (CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
        {
            const AWSGameLiftCreateSessionRequest& gameliftCreateSessionRequest =
                static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);
            result = CreateSessionHelper(gameliftCreateSessionRequest);
        }
        else if (CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(createSessionRequest))
        {
            const AWSGameLiftCreateSessionOnQueueRequest& gameliftCreateSessionOnQueueRequest =
                static_cast<const AWSGameLiftCreateSessionOnQueueRequest&>(createSessionRequest);
            result = CreateSessionOnQueueHelper(gameliftCreateSessionOnQueueRequest);
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
                [this, gameliftCreateSessionRequest]()
                {
                    AZStd::string result = CreateSessionHelper(gameliftCreateSessionRequest);

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
                [this, gameliftCreateSessionOnQueueRequest]()
                {
                    AZStd::string result = CreateSessionOnQueueHelper(gameliftCreateSessionOnQueueRequest);

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

    AZStd::string AWSGameLiftClientManager::CreateSessionHelper(
        const AWSGameLiftCreateSessionRequest& createSessionRequest)
    {
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameLiftClient = m_gameliftClient;
        AZStd::string result = "";
        if (!gameLiftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else
        {
            result = CreateSessionActivity::CreateSession(*gameLiftClient, createSessionRequest);
        }
        return result;
    }

    AZStd::string AWSGameLiftClientManager::CreateSessionOnQueueHelper(
        const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest)
    {
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient = m_gameliftClient;
        AZStd::string result;
        if (!gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else
        {
            result = CreateSessionOnQueueActivity::CreateSessionOnQueue(*gameliftClient, createSessionOnQueueRequest);
        }
        return result;
    }

    bool AWSGameLiftClientManager::JoinSession(const AzFramework::JoinSessionRequest& joinSessionRequest)
    {
        bool result = false;
        if (JoinSessionActivity::ValidateJoinSessionRequest(joinSessionRequest))
        {
            const AWSGameLiftJoinSessionRequest& gameliftJoinSessionRequest =
                static_cast<const AWSGameLiftJoinSessionRequest&>(joinSessionRequest);
            result = JoinSessionHelper(gameliftJoinSessionRequest);
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
                bool result = JoinSessionHelper(gameliftJoinSessionRequest);

                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, result);
            },
            true, jobContext);

        joinSessionJob->Start();
    }

    bool AWSGameLiftClientManager::JoinSessionHelper(const AWSGameLiftJoinSessionRequest& joinSessionRequest)
    {
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient = m_gameliftClient;
        bool result = false;
        if (!gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else
        {
            auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(*gameliftClient, joinSessionRequest);

            result = JoinSessionActivity::RequestPlayerJoinSession(createPlayerSessionOutcome);
        }
        return result;
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
            response = SearchSessionsHelper(gameliftSearchSessionsRequest);
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
                AzFramework::SearchSessionsResponse response = SearchSessionsHelper(gameliftSearchSessionsRequest);

                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnSearchSessionsAsyncComplete, response);
            },
            true, jobContext);

        searchSessionsJob->Start(); 
    }

    AzFramework::SearchSessionsResponse AWSGameLiftClientManager::SearchSessionsHelper(
        const AWSGameLiftSearchSessionsRequest& searchSessionsRequest) const
    {
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient = m_gameliftClient;

        AzFramework::SearchSessionsResponse response;
        if (!gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else
        {
            response = SearchSessionsActivity::SearchSessions(*gameliftClient, searchSessionsRequest);
        }
        return response;
    }

    void AWSGameLiftClientManager::SetGameLiftClient(AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient)
    {
        m_gameliftClient.swap(gameliftClient);
    }
} // namespace AWSGameLift
