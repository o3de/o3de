/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Profiler
{
    class ProfilerRequests
    {
    public:
        AZ_RTTI(ProfilerRequests, "{3757c4e5-1941-457c-85ae-16305e17a4c6}");
        virtual ~ProfilerRequests() = default;
        // Put your public methods here
    };
    
    class ProfilerBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ProfilerRequestBus = AZ::EBus<ProfilerRequests, ProfilerBusTraits>;
    using ProfilerInterface = AZ::Interface<ProfilerRequests>;

} // namespace Profiler
