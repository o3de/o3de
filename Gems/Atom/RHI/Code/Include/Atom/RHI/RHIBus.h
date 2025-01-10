/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PhysicalDevice.h>
#include <AzCore/EBus/EBus.h>

namespace AZ::RHI
{
    class Device;
    //! Ebus to collect requirements for creating a PhysicalDevice.
    class RHIRequirementsRequest : public AZ::EBusTraits
    {
    public:
        virtual ~RHIRequirementsRequest() = default;

        //! Removes PhysicalDevices that are not supported from a list of available devices.
        virtual void FilterSupportedPhysicalDevices(
            [[maybe_unused]] PhysicalDeviceList& supportedDevices, [[maybe_unused]] AZ::RHI::APIIndex apiIndex){};

        virtual size_t GetRequiredAlignment([[maybe_unused]] const Device& device)
        {
            return 0;
        };

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using MutexType = AZStd::mutex;
    };

    using RHIRequirementRequestBus = AZ::EBus<RHIRequirementsRequest>;
} // namespace AZ::RHI
