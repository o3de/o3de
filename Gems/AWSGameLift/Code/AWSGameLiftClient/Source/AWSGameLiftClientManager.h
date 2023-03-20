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
#include <Multiplayer/Session/MatchmakingNotifications.h>

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
        : public Multiplayer::MatchmakingNotificationBus::Handler
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
        : public Multiplayer::MatchmakingAsyncRequestNotificationBus::Handler
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
        : public Multiplayer::SessionAsyncRequestNotificationBus::Handler
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

        void OnSearchSessionsAsyncComplete(const Multiplayer::SearchSessionsResponse& searchSessionsResponse) override
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
        void AcceptMatchAsync(const Multiplayer::AcceptMatchRequest& acceptMatchRequest) override;
        void StartMatchmakingAsync(const Multiplayer::StartMatchmakingRequest& startMatchmakingRequest) override;
        void StopMatchmakingAsync(const Multiplayer::StopMatchmakingRequest& stopMatchmakingRequest) override;

        // AWSGameLiftSessionAsyncRequestBus interface implementation
        void CreateSessionAsync(const Multiplayer::CreateSessionRequest& createSessionRequest) override;
        void JoinSessionAsync(const Multiplayer::JoinSessionRequest& joinSessionRequest) override;
        void SearchSessionsAsync(const Multiplayer::SearchSessionsRequest& searchSessionsRequest) const override;
        void LeaveSessionAsync() override;

        // AWSGameLiftMatchmakingRequestBus interface implementation
        void AcceptMatch(const Multiplayer::AcceptMatchRequest& acceptMatchRequest) override;
        AZStd::string StartMatchmaking(const Multiplayer::StartMatchmakingRequest& startMatchmakingRequest) override;
        void StopMatchmaking(const Multiplayer::StopMatchmakingRequest& stopMatchmakingRequest) override;

        // AWSGameLiftSessionRequestBus interface implementation
        AZStd::string CreateSession(const Multiplayer::CreateSessionRequest& createSessionRequest) override;
        bool JoinSession(const Multiplayer::JoinSessionRequest& joinSessionRequest) override;
        Multiplayer::SearchSessionsResponse SearchSessions(const Multiplayer::SearchSessionsRequest& searchSessionsRequest) const override;
        void LeaveSession() override;
    };
} // namespace AWSGameLift
