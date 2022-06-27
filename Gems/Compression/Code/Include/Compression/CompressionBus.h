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

namespace Compression
{
    class CompressionRequests
    {
    public:
        AZ_RTTI(CompressionRequests, "{40076ED8-6AC5-498B-AF61-C5CD7136750E}");
        virtual ~CompressionRequests() = default;
        // Put your public methods here
    };
    
    class CompressionBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using CompressionRequestBus = AZ::EBus<CompressionRequests, CompressionBusTraits>;
    using CompressionInterface = AZ::Interface<CompressionRequests>;

} // namespace Compression
