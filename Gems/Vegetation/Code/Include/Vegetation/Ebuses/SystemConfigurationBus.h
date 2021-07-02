/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class ComponentConfig;
}

namespace Vegetation
{
    /**
    * A bus to update vegetation systems such as sector size and instancing slice time
    */
    class SystemConfigurationRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~SystemConfigurationRequests() = default;

        // update the internal configuration
        virtual void UpdateSystemConfig(const AZ::ComponentConfig* config) = 0;

        // fill out a specific configuration
        virtual void GetSystemConfig(AZ::ComponentConfig* config) const = 0;
    };

    using SystemConfigurationRequestBus = AZ::EBus<SystemConfigurationRequests>;

} // namespace Vegetation
