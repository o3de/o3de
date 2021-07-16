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
    //! AWSResourceMapping request interface
    class AWSResourceMappingRequests
        : public AZ::EBusTraits
    {
    public:
        // Allow multiple threads to concurrently make requests
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! GetDefaultAccountId
        //! Get default account id which is shared among resources
        //! @return Default account id in string
        virtual AZStd::string GetDefaultAccountId() const = 0;

        //! GetDefaultRegion
        //! Get default region which is shared among resources
        //! @return Default region in string
        virtual AZStd::string GetDefaultRegion() const = 0;

        //! GetResourceAccountId
        //! Get individual resource account id by using its mapping key name.
        //! If resource account id is not present in resource attributes, will use
        //! default account id instead
        //! @param resourceKeyName Resource mapping key name is used to identify individual
        //!        resource attributes
        //! @return Resource account id in string
        virtual AZStd::string GetResourceAccountId(const AZStd::string& resourceKeyName) const = 0;

        //! GetResourceNameId
        //! Get individual resource name/id by using its mapping key name
        //! @param resourceKeyName Resource mapping key name is used to identify individual
        //!        resource attributes
        //! @return Resource name/id in string
        virtual AZStd::string GetResourceNameId(const AZStd::string& resourceKeyName) const = 0;

        //! GetResourceRegion
        //! Get individual resource region by using its mapping key name.
        //! If resource region is not present in resource attributes, will use
        //! default region instead
        //! @param resourceKeyName Resource mapping key name is used to identify individual
        //!        resource attributes
        //! @return Resource region in string
        virtual AZStd::string GetResourceRegion(const AZStd::string& resourceKeyName) const = 0;

        //! GetResourceType
        //! Get individual resource type by using its mapping key name
        //! @param resourceKeyName Resource mapping key name is used to identify individual
        //!        resource attributes
        //! @return Resource type in string
        virtual AZStd::string GetResourceType(const AZStd::string& resourceKeyName) const = 0;

        //! GetServiceUrl
        //! Returns the base url for a registered APIGateway service endpoint
        //! @param serviceName The name of the Gem or mapping name that provides the services
        //! @return the service URL without a trailing / character
        virtual AZStd::string GetServiceUrlByServiceName(const AZStd::string& serviceName) const = 0;

        //! GetServiceUrl
        //! Returns the base url for a registered APIGateway service endpoint
        //! @param restApiIdKeyName The resource key name of APIGateway service REST Api id
        //! @param restApiStageKeyName The resource key name of APIGateway service REST Api stage
        //! @return the service URL without a trailing / character
        virtual AZStd::string GetServiceUrlByRESTApiIdAndStage(
            const AZStd::string& restApiIdKeyName, const AZStd::string& restApiStageKeyName) const = 0;

        //! ReloadConfigFile
        //! Reload resource mapping config file without restarting application
        //! @param isReloadingConfigFileName Whether reload resource mapping config file name
        //!        from AWS core configuration settings registry file
        virtual void ReloadConfigFile(bool isReloadingConfigFileName) = 0;
    };

    using AWSResourceMappingRequestBus = AZ::EBus<AWSResourceMappingRequests>;
} // namespace AWSCore
