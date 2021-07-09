/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Request/IAWSGameLiftRequests.h>

namespace Aws
{
    namespace GameLift
    {
        class GameLiftClient;
    }
}

namespace AWSGameLift
{
    struct AWSGameLiftCreateSessionRequest;
    struct AWSGameLiftCreateSessionOnQueueRequest;
    struct AWSGameLiftJoinSessionRequest;
    struct AWSGameLiftSearchSessionsRequest;

    // SessionAsyncRequestNotificationBus EBus handler for scripting
    class AWSGameLiftSessionAsyncRequestNotificationBusHandler
        : public AzFramework::SessionAsyncRequestNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            AWSGameLiftSessionAsyncRequestNotificationBusHandler,
            "{6E13FC73-53DC-4B6B-AEA7-9038DE4C9635}",
            AZ::SystemAllocator,
            OnCreateSessionAsyncComplete,
            OnSearchSessionsAsyncComplete,
            OnJoinSessionAsyncComplete,
            OnLeaveSessionAsyncComplete);

        void OnCreateSessionAsyncComplete(const AZStd::string& createSessionReponse) override
        {
            Call(FN_OnCreateSessionAsyncComplete, createSessionReponse);
        }

        void OnSearchSessionsAsyncComplete(const AzFramework::SearchSessionsResponse& searchSessionsResponse) override
        {
            Call(FN_OnSearchSessionsAsyncComplete, searchSessionsResponse);
        }

        void OnJoinSessionAsyncComplete(bool joinSessionsResponse) override
        {
            Call(FN_OnJoinSessionAsyncComplete, joinSessionsResponse);
        }

        void OnLeaveSessionAsyncComplete() override
        {
            Call(FN_OnLeaveSessionAsyncComplete);
        }
    };

    //! AWSGameLiftClientManager
    //! GameLift client manager to support game and player session related client requests
    class AWSGameLiftClientManager
        : public AWSGameLiftRequestBus::Handler
        , public AWSGameLiftSessionAsyncRequestBus::Handler
        , public AWSGameLiftSessionRequestBus::Handler
    {
    public:
        static constexpr const char AWSGameLiftClientManagerName[] = "AWSGameLiftClientManager";
        static constexpr const char AWSGameLiftClientRegionMissingErrorMessage[] =
            "Missing AWS region for GameLift client.";
        static constexpr const char AWSGameLiftClientCredentialMissingErrorMessage[] =
            "Missing AWS credential for GameLift client.";
        static constexpr const char AWSGameLiftClientMissingErrorMessage[] =
            "GameLift client is not configured yet.";

        static constexpr const char AWSGameLiftCreateSessionRequestInvalidErrorMessage[] =
            "Invalid GameLift CreateSession or CreateSessionOnQueue request.";

        AWSGameLiftClientManager();
        virtual ~AWSGameLiftClientManager() = default;

        virtual void ActivateManager();
        virtual void DeactivateManager();

        // AWSGameLiftRequestBus interface implementation
        bool ConfigureGameLiftClient(const AZStd::string& region) override;
        AZStd::string CreatePlayerId(bool includeBrackets, bool includeDashes) override;

        // AWSGameLiftSessionAsyncRequestBus interface implementation
        void CreateSessionAsync(const AzFramework::CreateSessionRequest& createSessionRequest) override;
        void JoinSessionAsync(const AzFramework::JoinSessionRequest& joinSessionRequest) override;
        void SearchSessionsAsync(const AzFramework::SearchSessionsRequest& searchSessionsRequest) const override;
        void LeaveSessionAsync() override;

        // AWSGameLiftSessionRequestBus interface implementation
        AZStd::string CreateSession(const AzFramework::CreateSessionRequest& createSessionRequest) override;
        bool JoinSession(const AzFramework::JoinSessionRequest& joinSessionRequest) override;
        AzFramework::SearchSessionsResponse SearchSessions(const AzFramework::SearchSessionsRequest& searchSessionsRequest) const override;
        void LeaveSession() override;

    protected:
        // Use for automation tests only to inject mock objects. 
        void SetGameLiftClient(AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient);

    private:
        AZStd::string CreateSessionHelper(const AWSGameLiftCreateSessionRequest& createSessionRequest);
        AZStd::string CreateSessionOnQueueHelper(const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest);
        bool JoinSessionHelper(const AWSGameLiftJoinSessionRequest& joinSessionRequest);
        AzFramework::SearchSessionsResponse SearchSessionsHelper(const AWSGameLiftSearchSessionsRequest& searchSessionsRequest) const;

        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> m_gameliftClient;
    };
} // namespace AWSGameLift
