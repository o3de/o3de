/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Matchmaking/IMatchmakingRequests.h>
#include <AzFramework/Session/ISessionRequests.h>

namespace AWSGameLift
{
    //! IAWSGameLiftRequests
    //! GameLift Gem interfaces to configure client manager
    class IAWSGameLiftRequests
    {
    public:
        AZ_RTTI(IAWSGameLiftRequests, "{494167AD-1185-4AF3-8BF9-C8C37FC9C199}");

        IAWSGameLiftRequests() = default;
        virtual ~IAWSGameLiftRequests() = default;

        //! ConfigureGameLiftClient
        //! Configure GameLift client to interact with Amazon GameLift service
        //! @param region Specifies the AWS region to use
        //! @return True if client configuration succeeds, false otherwise
        virtual bool ConfigureGameLiftClient(const AZStd::string& region) = 0;

        //! CreatePlayerId
        //! Create a new, random ID number for every player in every new game session.
        //! @param includeBrackets Whether includes brackets in player id
        //! @param includeDashes Whether includes dashes in player id
        //! @return The player id to use in game session
        virtual AZStd::string CreatePlayerId(bool includeBrackets, bool includeDashes) = 0;
    };

    // IAWSGameLiftRequests EBus wrapper for scripting
    class AWSGameLiftRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftRequestBus = AZ::EBus<IAWSGameLiftRequests, AWSGameLiftRequests>;

    // ISessionAsyncRequests EBus wrapper for scripting
    class AWSGameLiftSessionAsyncRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftSessionAsyncRequestBus = AZ::EBus<AzFramework::ISessionAsyncRequests, AWSGameLiftSessionAsyncRequests>;

    // ISessionRequests EBus wrapper for scripting
    class AWSGameLiftSessionRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftSessionRequestBus = AZ::EBus<AzFramework::ISessionRequests, AWSGameLiftSessionRequests>;

    // IMatchmakingAsyncRequests EBus wrapper for scripting
    class AWSGameLiftMatchmakingAsyncRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftMatchmakingAsyncRequestBus = AZ::EBus<AzFramework::IMatchmakingAsyncRequests, AWSGameLiftMatchmakingAsyncRequests>;

    // IMatchmakingRequests EBus wrapper for scripting
    class AWSGameLiftMatchmakingRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftMatchmakingRequestBus = AZ::EBus<AzFramework::IMatchmakingRequests, AWSGameLiftMatchmakingRequests>;

    //! IAWSGameLiftMatchmakingEventRequests
    //! GameLift Gem matchmaking event interfaces which is used to track matchmaking ticket event
    //! Developer should define the way to poll matchmaking ticket event and behavior based on the ticket status
    //! Use AWSGameLiftClientLocalTicketTracker as an example, it uses continuous polling to query matchmaking ticket:
    //! StartPolling - local ticket tracker starts monitor process for matchmaking ticket, and joins player
    //!                to the match once ticket is complete
    //! StopPolling - local ticket tracker cancels ongoing matchmaking ticket and stops monitoring process
    class IAWSGameLiftMatchmakingEventRequests
    {
    public:
        AZ_RTTI(IAWSGameLiftMatchmakingEventRequests, "{C2DA440E-74E0-411E-813D-5880B50B0C9E}");

        IAWSGameLiftMatchmakingEventRequests() = default;
        virtual ~IAWSGameLiftMatchmakingEventRequests() = default;

        //! StartPolling
        //! Request to start process for polling matchmaking ticket based on given ticket id and player Id
        //! @param ticketId The requested matchmaking ticket id
        //! @param playerId The requested matchmaking player id
        virtual void StartPolling(const AZStd::string& ticketId, const AZStd::string& playerId) = 0;

        //! StopPolling
        //! Request to stop process for polling matchmaking ticket
        virtual void StopPolling() = 0;
    };

    // IAWSGameLiftMatchmakingEventRequests EBus wrapper for scripting
    class AWSGameLiftMatchmakingEventRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftMatchmakingEventRequestBus = AZ::EBus<IAWSGameLiftMatchmakingEventRequests, AWSGameLiftMatchmakingEventRequests>;
} // namespace AWSGameLift
