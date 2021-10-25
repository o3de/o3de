/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Matchmaking/MatchmakingNotifications.h>

#include <Request/AWSGameLiftRequestBus.h>
#include <Request/AWSGameLiftSessionRequestBus.h>
#include <Request/AWSGameLiftMatchmakingRequestBus.h>

namespace AWSGameLift
{
    struct AWSGameLiftAcceptMatchRequest;
    struct AWSGameLiftCreateSessionRequest;
    struct AWSGameLiftCreateSessionOnQueueRequest;
    struct AWSGameLiftJoinSessionRequest;
    struct AWSGameLiftSearchSessionsRequest;
    struct AWSGameLiftStartMatchmakingRequest;
    struct AWSGameLiftStopMatchmakingRequest;

    // MatchmakingNotificationBus EBus handler for scripting
    class AWSGameLiftMatchmakingNotificationBusHandler
        : public AzFramework::MatchmakingNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            AWSGameLiftMatchmakingNotificationBusHandler,
            "{CBE057D3-F5CE-46D3-B02D-8A6A1446B169}",
            AZ::SystemAllocator,
            OnMatchAcceptance, OnMatchComplete, OnMatchError, OnMatchFailure);

        void OnMatchAcceptance() override
        {
            Call(FN_OnMatchAcceptance);
        }

        void OnMatchComplete() override
        {
            Call(FN_OnMatchComplete);
        }

        void OnMatchError() override
        {
            Call(FN_OnMatchError);
        }

        void OnMatchFailure() override
        {
            Call(FN_OnMatchFailure);
        }
    };

    // MatchmakingAsyncRequestNotificationBus EBus handler for scripting
    class AWSGameLiftMatchmakingAsyncRequestNotificationBusHandler
        : public AzFramework::MatchmakingAsyncRequestNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            AWSGameLiftMatchmakingAsyncRequestNotificationBusHandler,
            "{2045EE8F-2AB7-4ED0-9614-3496A1A43677}",
            AZ::SystemAllocator,
            OnAcceptMatchAsyncComplete,
            OnStartMatchmakingAsyncComplete,
            OnStopMatchmakingAsyncComplete);

        void OnAcceptMatchAsyncComplete() override
        {
            Call(FN_OnAcceptMatchAsyncComplete);
        }

        void OnStartMatchmakingAsyncComplete(const AZStd::string& matchmakingTicketId) override
        {
            Call(FN_OnStartMatchmakingAsyncComplete, matchmakingTicketId);
        }

        void OnStopMatchmakingAsyncComplete() override
        {
            Call(FN_OnStopMatchmakingAsyncComplete);
        }
    };

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
        , public AWSGameLiftMatchmakingAsyncRequestBus::Handler
        , public AWSGameLiftMatchmakingRequestBus::Handler
        , public AWSGameLiftSessionAsyncRequestBus::Handler
        , public AWSGameLiftSessionRequestBus::Handler
    {
    public:
        static constexpr const char AWSGameLiftClientManagerName[] = "AWSGameLiftClientManager";
        static constexpr const char AWSGameLiftClientRegionMissingErrorMessage[] =
            "Missing AWS region for GameLift client.";
        static constexpr const char AWSGameLiftClientCredentialMissingErrorMessage[] =
            "Missing AWS credential for GameLift client.";

        static constexpr const char AWSGameLiftCreateSessionRequestInvalidErrorMessage[] =
            "Invalid GameLift CreateSession or CreateSessionOnQueue request.";

        AWSGameLiftClientManager() = default;
        virtual ~AWSGameLiftClientManager() = default;

        virtual void ActivateManager();
        virtual void DeactivateManager();

        // AWSGameLiftRequestBus interface implementation
        bool ConfigureGameLiftClient(const AZStd::string& region) override;
        AZStd::string CreatePlayerId(bool includeBrackets, bool includeDashes) override;

        // AWSGameLiftMatchmakingAsyncRequestBus interface implementation
        void AcceptMatchAsync(const AzFramework::AcceptMatchRequest& acceptMatchRequest) override;
        void StartMatchmakingAsync(const AzFramework::StartMatchmakingRequest& startMatchmakingRequest) override;
        void StopMatchmakingAsync(const AzFramework::StopMatchmakingRequest& stopMatchmakingRequest) override;

        // AWSGameLiftSessionAsyncRequestBus interface implementation
        void CreateSessionAsync(const AzFramework::CreateSessionRequest& createSessionRequest) override;
        void JoinSessionAsync(const AzFramework::JoinSessionRequest& joinSessionRequest) override;
        void SearchSessionsAsync(const AzFramework::SearchSessionsRequest& searchSessionsRequest) const override;
        void LeaveSessionAsync() override;

        // AWSGameLiftMatchmakingRequestBus interface implementation
        void AcceptMatch(const AzFramework::AcceptMatchRequest& acceptMatchRequest) override;
        AZStd::string StartMatchmaking(const AzFramework::StartMatchmakingRequest& startMatchmakingRequest) override;
        void StopMatchmaking(const AzFramework::StopMatchmakingRequest& stopMatchmakingRequest) override;

        // AWSGameLiftSessionRequestBus interface implementation
        AZStd::string CreateSession(const AzFramework::CreateSessionRequest& createSessionRequest) override;
        bool JoinSession(const AzFramework::JoinSessionRequest& joinSessionRequest) override;
        AzFramework::SearchSessionsResponse SearchSessions(const AzFramework::SearchSessionsRequest& searchSessionsRequest) const override;
        void LeaveSession() override;

    private:
        void AcceptMatchHelper(const AWSGameLiftAcceptMatchRequest& createSessionRequest);
        AZStd::string CreateSessionHelper(const AWSGameLiftCreateSessionRequest& createSessionRequest);
        AZStd::string CreateSessionOnQueueHelper(const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest);
        bool JoinSessionHelper(const AWSGameLiftJoinSessionRequest& joinSessionRequest);
        AzFramework::SearchSessionsResponse SearchSessionsHelper(const AWSGameLiftSearchSessionsRequest& searchSessionsRequest) const;
        AZStd::string StartMatchmakingHelper(const AWSGameLiftStartMatchmakingRequest& startMatchmakingRequest);
        void StopMatchmakingHelper(const AWSGameLiftStopMatchmakingRequest& stopMatchmakingRequest);
    };
} // namespace AWSGameLift
