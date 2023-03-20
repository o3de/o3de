/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AWSGameLift
{
    //! IAWSGameLiftRequests
    //! GameLift Gem interfaces to configure GameLift client and other help functions,
    //! like creating random GameLift player id
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

    // IAWSGameLiftRequests EBus wrapper
    class AWSGameLiftRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftRequestBus = AZ::EBus<IAWSGameLiftRequests, AWSGameLiftRequests>;
} // namespace AWSGameLift
