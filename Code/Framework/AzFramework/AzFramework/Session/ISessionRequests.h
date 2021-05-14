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

#pragma once

#include <AzCore/EBus/EBus.h>
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
        // A collection of sessions that match the search criteria and sorted in specific order.
        AZStd::vector<SessionConfig> m_sessionConfigs;

        // A token that indicates the start of the next sequential page of results.
        AZStd::string m_nextToken;
    };

    //! JoinSessionRequest
    //! The container for JoinSession request parameters.
    struct JoinSessionRequest
    {
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
        // Create a session for players to find and join.
        // @param  createSessionRequest The request of CreateSession operation
        // @return Outcome of CreateSession operation
        virtual AZ::Outcome<void, AZStd::string> CreateSession(const CreateSessionRequest& createSessionRequest) = 0;

        // CreateSession Async
        // @param  createSessionRequest The request of CreateSession operation
        virtual void CreateSessionAsync(const CreateSessionRequest& createSessionRequest) = 0;

        // Retrieve all active sessions that match the given search criteria and sorted in specific order.
        // @param  searchSessionsRequest The request of SearchSessions operation
        // @return The response of SearchSessions operation
        virtual SearchSessionsResponse SearchSessions(const SearchSessionsRequest& searchSessionsRequest) const = 0;

        // SearchSessions Async
        // @param  searchSessionsRequest The request of SearchSessions operation
        virtual void SearchSessionsAsync(const SearchSessionsRequest& searchSessionsRequest) const = 0;

        // Reserve an open player slot in a session, and perform connection from client to server.
        // @param  joinSessionRequest The request of JoinSession operation
        // @return Outcome of JoinSession operation
        virtual AZ::Outcome<void, AZStd::string> JoinSession(const JoinSessionRequest& joinSessionRequest) = 0;

        // JoinSession Async
        // @param  joinSessionRequest The request of JoinSession operation
        virtual void JoinSessionAsync(const JoinSessionRequest& joinSessionRequest) = 0;

        // Disconnect player from session.
        virtual void LeaveSession() = 0;
    };

    //! SessionAsyncRequestNotifications
    //! The notifications correspond to session async requests
    class SessionAsyncRequestNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // OnCreateSessionAsyncComplete is fired once CreateSessionAsync completes
        // @param createSessionResponse The response of CreateSession call
        virtual void OnCreateSessionAsyncComplete(const AZ::Outcome<void, AZStd::string>& createSessionResponse) = 0;

        // OnSearchSessionsAsyncComplete is fired once SearchSessionsAsync completes
        // @param searchSessionsResponse The response of SearchSessions call
        virtual void OnSearchSessionsAsyncComplete(const SearchSessionsResponse& searchSessionsResponse) = 0;

        // OnJoinSessionAsyncComplete is fired once JoinSessionAsync completes
        // @param joinSessionsResponse The response of JoinSession call
        virtual void OnJoinSessionAsyncComplete(const AZ::Outcome<void, AZStd::string>& joinSessionsResponse) = 0;
    };
} // namespace AzFramework
