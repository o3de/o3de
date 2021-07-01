/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetProcessor
{
    class BuilderConfigurationRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        virtual ~BuilderConfigurationRequests() = default;

        //! Load configuration data from a specific BuilderConfig.ini file
        virtual bool LoadConfiguration(const AZStd::string& /*configFile*/) { return false; }

        //! Update a job descriptor given the configuration data which has been loaded
        virtual bool UpdateJobDescriptor(const AZStd::string& /*jobKey*/, AssetBuilderSDK::JobDescriptor& /*jobDesc*/) { return false; }

        //! Update a builder desc given configuration data
        virtual bool UpdateBuilderDescriptor(const AZStd::string& /*builderName*/, AssetBuilderSDK::AssetBuilderDesc& /*jobDesc*/) { return false; }
    };

    using BuilderConfigurationRequestBus = AZ::EBus<BuilderConfigurationRequests>;


} // namespace AssetProcessor


