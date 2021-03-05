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