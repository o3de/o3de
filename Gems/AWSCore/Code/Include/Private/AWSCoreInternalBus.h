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

        //! GetResourceMappingConfigFolderPath
        //! Get the path of AWS resource mapping config folder
        //! @return The path of AWS resource mapping config folder
        virtual AZStd::string GetResourceMappingConfigFolderPath() const = 0;

        //! ReloadConfiguration
        //! Reload AWSCore configuration without restarting application
        virtual void ReloadConfiguration() = 0;
    };

    using AWSCoreInternalRequestBus = AZ::EBus<AWSCoreInternalRequests>;
} // namespace AWSCore
