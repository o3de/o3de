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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>
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
} // namespace AWSGameLift
