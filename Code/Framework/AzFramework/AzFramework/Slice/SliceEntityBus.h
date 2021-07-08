/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Slice/SliceComponent.h>

namespace AzFramework
{
    /**
     * Interface for AzFramework::SliceEntityRequestBus, which can be used to request information about the owning slices of entities.
     */
    class SliceEntityRequests
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~SliceEntityRequests() = default;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        using BusIdType = AZ::EntityId;

        /**
         * Gets the address of the slice instance that owns the entity.
         * 
         * @return SliceInstanceAddress
         */
        virtual AZ::SliceComponent::SliceInstanceAddress GetOwningSlice() = 0;
    };

    using SliceEntityRequestBus = AZ::EBus<SliceEntityRequests>;
}
