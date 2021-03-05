/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
