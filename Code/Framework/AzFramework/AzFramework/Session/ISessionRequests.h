/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>

namespace AzFramework
{
    struct SessionConfig;

    //! CreateSessionRequest
    //! The container for CreateSession request parameters.
    struct CreateSessionRequest
    {
        AZ_RTTI(CreateSessionRequest, "{E39C2A45-89C9-4CFB-B337-9734DC798930}");
        static void Reflect(AZ::ReflectContext* context);

        CreateSessionRequest() = default;
        virtual ~CreateSessionRequest() = default;

        // A unique identifier for a player or entity creating the session.
        AZStd::string m_creatorId;

        // A collection of custom properties for a session.
        AZStd::unordered_map<AZStd::string, AZStd::string> m_sessionProperties;

        // A descriptive label that is associated with a session.
        AZStd::string m_sessionName;

        // The maximum number of players that can be connected simultaneously to the session.
        uint64_t m_maxPlayer;
    };

    //! SearchSessionsRequest
    //! The container for SearchSessions request parameters.
    struct SearchSessionsRequest
    {
        AZ_RTTI(SearchSessionsRequest, "{B49207A8-8549-4ADB-B7D9-D7A4932F9B4B}");
        static void Reflect(AZ::ReflectContext* context);

        SearchSessionsRequest() = default;
        virtual ~SearchSessionsRequest() = default;

        // String containing the search criteria for the session search. If no filter expression is included, the request returns results
        // for all active sessions.
        AZStd::string m_filterExpression;

        // Instructions on how to sort the search results. If no sort expression is included, the request returns results in random order.
        AZStd::string m_sortExpression;

        // The maximum number of results to return.
        uint8_t m_maxResult;

        // A token that indicates the start of the next sequential page of results.
        AZStd::string m_nextToken;
    };

    //! SearchSessionsResponse
    //! The container for SearchSession request results.
    struct SearchSessionsResponse
    {
        AZ_RTTI(SearchSessionsResponse, "{F93DE7DC-D381-4E08-8A3B-0B08F7C38714}");
        static void Reflect(AZ::ReflectContext* context);

        SearchSessionsResponse() = default;
        virtual ~SearchSessionsResponse() = default;

        // A collection of sessions that match the search criteria and sorted in specific order.
        AZStd::vector<SessionConfig> m_sessionConfigs;

        // A token that indicates the start of the next sequential page of results.
        AZStd::string m_nextToken;
    };

    //! JoinSessionRequest
    //! The container for JoinSession request parameters.
    struct JoinSessionRequest
    {
        AZ_RTTI(JoinSessionRequest, "{519769E8-3CDE-4385-A0D7-24DBB3685657}");
        static void Reflect(AZ::ReflectContext* context);

        JoinSessionRequest() = default;
        virtual ~JoinSessionRequest() = default;

        // A unique identifier for the session.
        AZStd::string m_sessionId;

        // A unique identifier for a player. Player IDs are developer-defined.
        AZStd::string m_playerId;

        // Developer-defined information related to a player.
        AZStd::string m_playerData;
    };

    //! ISessionRequests
    //! Pure virtual session interface class to abstract the details of session handling from application code.
    class ISessionRequests
    {
    public:
        AZ_RTTI(ISessionRequests, "{D6C41A71-DD8D-47FE-8515-FAF90670AE2F}");

        ISessionRequests() = default;
        virtual ~ISessionRequests() = default;

        // Create a session for players to find and join.
        // @param  createSessionRequest The request of CreateSession operation
        // @return The request id if session creation request succeeds; empty if it fails
        virtual AZStd::string CreateSession(const CreateSessionRequest& createSessionRequest) = 0;

        // Retrieve all active sessions that match the given search criteria and sorted in specific order.
        // @param  searchSessionsRequest The request of SearchSessions operation
        // @return The response of SearchSessions operation
        virtual SearchSessionsResponse SearchSessions(const SearchSessionsRequest& searchSessionsRequest) const = 0;

        // Reserve an open player slot in a session, and perform connection from client to server.
        // @param  joinSessionRequest The request of JoinSession operation
        // @return True if joining session succeeds; False otherwise
        virtual bool JoinSession(const JoinSessionRequest& joinSessionRequest) = 0;

        // Disconnect player from session.
        virtual void LeaveSession() = 0;
    };

    //! ISessionAsyncRequests
    //! Async version of ISessionRequests
    class ISessionAsyncRequests
    {
    public:
        AZ_RTTI(ISessionAsyncRequests, "{471542AF-96B9-4930-82FE-242A4E68432D}");

        ISessionAsyncRequests() = default;
        virtual ~ISessionAsyncRequests() = default;

        // CreateSession Async
        // @param  createSessionRequest The request of CreateSession operation
        virtual void CreateSessionAsync(const CreateSessionRequest& createSessionRequest) = 0;

        // SearchSessions Async
        // @param  searchSessionsRequest The request of SearchSessions operation
        virtual void SearchSessionsAsync(const SearchSessionsRequest& searchSessionsRequest) const = 0;

        // JoinSession Async
        // @param  joinSessionRequest The request of JoinSession operation
        virtual void JoinSessionAsync(const JoinSessionRequest& joinSessionRequest) = 0;

        // LeaveSession Async
        virtual void LeaveSessionAsync() = 0;
    };

    //! SessionAsyncRequestNotifications
    //! The notifications correspond to session async requests
    class SessionAsyncRequestNotifications
        : public AZ::EBusTraits
    {
    public:
        // Safeguard handler for multi-threaded use case
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // OnCreateSessionAsyncComplete is fired once CreateSessionAsync completes
        // @param createSessionResponse The request id if session creation request succeeds; empty if it fails
        virtual void OnCreateSessionAsyncComplete(const AZStd::string& createSessionReponse) = 0;

        // OnSearchSessionsAsyncComplete is fired once SearchSessionsAsync completes
        // @param searchSessionsResponse The response of SearchSessions call
        virtual void OnSearchSessionsAsyncComplete(const SearchSessionsResponse& searchSessionsResponse) = 0;

        // OnJoinSessionAsyncComplete is fired once JoinSessionAsync completes
        // @param joinSessionsResponse True if joining session succeeds; False otherwise
        virtual void OnJoinSessionAsyncComplete(bool joinSessionsResponse) = 0;

        // OnLeaveSessionAsyncComplete is fired once LeaveSessionAsync completes
        virtual void OnLeaveSessionAsyncComplete() = 0;
    };
    using SessionAsyncRequestNotificationBus = AZ::EBus<SessionAsyncRequestNotifications>;
} // namespace AzFramework
