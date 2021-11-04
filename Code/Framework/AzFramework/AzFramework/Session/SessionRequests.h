/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

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

        //! A unique identifier for a player or entity creating the session.
        AZStd::string m_creatorId;

        //! A collection of custom properties for a session.
        AZStd::unordered_map<AZStd::string, AZStd::string> m_sessionProperties;

        //! A descriptive label that is associated with a session.
        AZStd::string m_sessionName;

        //! The maximum number of players that can be connected simultaneously to the session.
        uint64_t m_maxPlayer = 0;
    };

    //! SearchSessionsRequest
    //! The container for SearchSessions request parameters.
    struct SearchSessionsRequest
    {
        AZ_RTTI(SearchSessionsRequest, "{B49207A8-8549-4ADB-B7D9-D7A4932F9B4B}");
        static void Reflect(AZ::ReflectContext* context);

        SearchSessionsRequest() = default;
        virtual ~SearchSessionsRequest() = default;

        //! String containing the search criteria for the session search. If no filter expression is included, the request returns results
        //! for all active sessions.
        AZStd::string m_filterExpression;

        //! Instructions on how to sort the search results. If no sort expression is included, the request returns results in random order.
        AZStd::string m_sortExpression;

        //! The maximum number of results to return.
        uint8_t m_maxResult = 0;

        //! A token that indicates the start of the next sequential page of results.
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

        //! A collection of sessions that match the search criteria and sorted in specific order.
        AZStd::vector<SessionConfig> m_sessionConfigs;

        //! A token that indicates the start of the next sequential page of results.
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

        //! A unique identifier for the session.
        AZStd::string m_sessionId;

        //! A unique identifier for a player. Player IDs are developer-defined.
        AZStd::string m_playerId;

        //! Developer-defined information related to a player.
        AZStd::string m_playerData;
    };
} // namespace AzFramework
