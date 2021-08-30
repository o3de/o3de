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
#include <AzFramework/Session/ISessionRequests.h>

namespace AWSGameLift
{
    //! IAWSGameLiftServerRequests
    //! Server interfaces to expose Amazon GameLift Server SDK
    class IAWSGameLiftServerRequests
    {
    public:
        AZ_RTTI(IAWSGameLiftServerRequests, "{D76CD98D-4C37-4C25-82C4-4E8772706D70}");

        IAWSGameLiftServerRequests() = default;
        virtual ~IAWSGameLiftServerRequests() = default;

        //! Notify GameLift that the server process is ready to host a game session.
        //! @return Whether the ProcessReady notification is sent to GameLift.
        virtual bool NotifyGameLiftProcessReady() = 0;
    };

    // IAWSGameLiftServerRequests EBus wrapper for scripting
    class AWSGameLiftServerRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftServerRequestBus = AZ::EBus<IAWSGameLiftServerRequests, AWSGameLiftServerRequests>;
} // namespace AWSGameLift
