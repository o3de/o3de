/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Session/SessionRequests.h>

namespace AzFramework
{
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
