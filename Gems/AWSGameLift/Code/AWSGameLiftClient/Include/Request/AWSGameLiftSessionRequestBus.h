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
#include <AzFramework/Session/ISessionRequests.h>

namespace AWSGameLift
{
    // ISessionAsyncRequests EBus wrapper
    class AWSGameLiftSessionAsyncRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using AWSGameLiftSessionAsyncRequestBus = AZ::EBus<AzFramework::ISessionAsyncRequests, AWSGameLiftSessionAsyncRequests>;

    // ISessionRequests EBus wrapper
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
