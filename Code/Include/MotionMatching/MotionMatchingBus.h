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

namespace EMotionFX::MotionMatching
{
    class MotionMatchingRequests
    {
    public:
        AZ_RTTI(MotionMatchingRequests, "{b08f73cc-a922-49ef-8c0e-07166b43ea65}");
        virtual ~MotionMatchingRequests() = default;
        // Put your public methods here
    };
    
    class MotionMatchingBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using MotionMatchingRequestBus = AZ::EBus<MotionMatchingRequests, MotionMatchingBusTraits>;
    using MotionMatchingInterface = AZ::Interface<MotionMatchingRequests>;

} // namespace EMotionFX::MotionMatching
