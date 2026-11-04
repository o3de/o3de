/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Stars/StarsTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AZ::Render
{
    class StarsRequests
    {
    public:
        AZ_RTTI(StarsRequests, StarsRequestsTypeId);
        virtual ~StarsRequests() = default;
    };

    class StarsBusTraits : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using StarsRequestBus = AZ::EBus<StarsRequests, StarsBusTraits>;
    using StarsInterface = AZ::Interface<StarsRequests>;

} // namespace AZ::Render
