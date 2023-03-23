/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CompressionLZ4/CompressionLZ4TypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace CompressionLZ4
{
    class CompressionLZ4Requests
    {
    public:
        AZ_RTTI(CompressionLZ4Requests, CompressionLZ4RequestsTypeId);
        virtual ~CompressionLZ4Requests() = default;
        // Put your public methods here
    };

    class CompressionLZ4BusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using CompressionLZ4RequestBus = AZ::EBus<CompressionLZ4Requests, CompressionLZ4BusTraits>;
    using CompressionLZ4Interface = AZ::Interface<CompressionLZ4Requests>;

} // namespace CompressionLZ4
