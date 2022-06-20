/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AWSCore
{
    //! AWSCoreInternal request interface
    class AWSCoreInternalRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! GetProfileName
        //! Get the AWS profile name for AWS credential
        //! @return The name of AWS profile
        virtual AZStd::string GetProfileName() const = 0;

        //! GetResourceMappingConfigFilePath
        //! Get the path of AWS resource mapping config file
        //! @return The path of AWS resource mapping config file
        virtual AZStd::string GetResourceMappingConfigFilePath() const = 0;

        //! ReloadConfiguration
        //! Reload AWSCore configuration without restarting application
        virtual void ReloadConfiguration() = 0;
    };

    using AWSCoreInternalRequestBus = AZ::EBus<AWSCoreInternalRequests>;
} // namespace AWSCore
