/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/ServiceJobConfig.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

namespace AWSCore
{

    /// Provides configuration needed by service jobs.
    class IServiceClientJobConfig
        : public virtual IServiceJobConfig
    {

    public:
        virtual AZStd::string GetServiceUrl() = 0;
    };

    /// Encapsulates what code needs to know about a service in order to 
    /// use it with a service job. Use the DEFINE_SERVICE_TRAITS macro
    /// to simplify the definition of these types.
    ///
    /// \param ServiceTraitsType - must implement the following static
    /// functions:
    ///
    ///    const char* GetServiceName();
    ///    const char* GetRESTApiIdKeyName();
    ///    const char* GetRESTApiStageKeyName();
    ///
    template<class ServiceTraitsType>
    class ServiceTraits
    {
    public:
        static const char* ServiceName;
        static const char* RESTApiIdKeyName;
        static const char* RESTApiStageKeyName;
    };

    template<class ServiceTraitsType>
    const char* ServiceTraits<ServiceTraitsType>::ServiceName = ServiceTraitsType::GetServiceName();
    template<class ServiceTraitsType>
    const char* ServiceTraits<ServiceTraitsType>::RESTApiIdKeyName = ServiceTraitsType::GetRESTApiIdKeyName();
    template<class ServiceTraitsType>
    const char* ServiceTraits<ServiceTraitsType>::RESTApiStageKeyName = ServiceTraitsType::GetRESTApiStageKeyName();

    #define AWS_SERVICE_TRAITS_TEMPLATE(SERVICE_NAME, RESTAPI_ID, RESTAPI_STAGE) \
        class SERVICE_NAME##ServiceTraits : public AWSCore::ServiceTraits<SERVICE_NAME##ServiceTraits> \
        { \
        private: \
            friend class AWSCore::ServiceTraits<SERVICE_NAME##ServiceTraits>; \
            static const char* GetServiceName() { return #SERVICE_NAME; } \
            static const char* GetRESTApiIdKeyName() { return RESTAPI_ID; } \
            static const char* GetRESTApiStageKeyName() { return RESTAPI_STAGE; } \
        };

    // warning C4250: 'AWSCore::ServiceClientJobConfig<ServiceTraitsType>' : inherits 'AWSCore::AwsApiJobConfig::AWSCore::AwsApiJobConfig::GetJobContext' via dominance
    // Thanks to http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors for the explanation
    // This is the expected and desired behavior. The warning is superfluous.
    AZ_PUSH_DISABLE_WARNING(4250, "-Wunknown-warning-option")
    /// Provides service job configuration using settings properties.
    template<class ServiceTraitsType>
    class ServiceClientJobConfig
        : public ServiceJobConfig
        , public virtual IServiceClientJobConfig
    {

    public:

        AZ_CLASS_ALLOCATOR(ServiceClientJobConfig, AZ::SystemAllocator, 0);

        using ServiceClientJobConfigType = ServiceClientJobConfig<ServiceTraitsType>;

        using InitializerFunction = AZStd::function<void(ServiceClientJobConfig& config)>;

        /// Initialize an ServiceClientJobConfig object.
        ///
        /// \param defaultConfig - the config object that provides values when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        ServiceClientJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : ServiceJobConfig{ defaultConfig }
        {
            if (initializer)
            {
                initializer(*this);
            }
        }

        /// This implementation assumes the caller will cache this value as 
        /// needed. See it's use in ServiceRequestJobConfig.
        AZStd::string GetServiceUrl() override
        {
            if (endpointOverride.has_value())
            {
                return endpointOverride.value().c_str();
            }

            AZStd::string serviceUrl;
            
            if (!ServiceTraitsType::RESTApiIdKeyName && !ServiceTraitsType::RESTApiStageKeyName)
            {
                AWSResourceMappingRequestBus::BroadcastResult(
                    serviceUrl, &AWSResourceMappingRequests::GetServiceUrlByServiceName,
                    ServiceTraitsType::ServiceName);
            }
            else
            {
                AWSResourceMappingRequestBus::BroadcastResult(
                    serviceUrl, &AWSResourceMappingRequests::GetServiceUrlByRESTApiIdAndStage,
                    ServiceTraitsType::RESTApiIdKeyName, ServiceTraitsType::RESTApiStageKeyName);
            }
            return serviceUrl;
        }

    };
    AZ_POP_DISABLE_WARNING

} // namespace AWSCore


