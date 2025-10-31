/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AFR/AFRTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AFR
{
    class AFRRequests
    {
    public:
        AZ_RTTI(AFRRequests, AFRRequestsTypeId);
        virtual ~AFRRequests() = default;
        // Put your public methods here
    };

    class AFRBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using AFRRequestBus = AZ::EBus<AFRRequests, AFRBusTraits>;
    using AFRInterface = AZ::Interface<AFRRequests>;

} // namespace AFR
