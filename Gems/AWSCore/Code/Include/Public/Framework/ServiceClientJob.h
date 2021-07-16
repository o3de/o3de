/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/ServiceJob.h>
#include <Framework/ServiceClientJobConfig.h>

namespace AWSCore
{
    /// Base class for AWSCore service request jobs. This class
    /// exists so that we have somewhere to put service type specific 
    /// configuration.
    template<class ServiceTraitsType>
    class ServiceClientJob
        : public ServiceJob
    {

    public:

        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(ServiceClientJob, AZ::SystemAllocator, 0);

        using IConfig = IServiceClientJobConfig;
        using Config = ServiceClientJobConfig<ServiceTraitsType>;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(ServiceJob::GetDefaultConfig());
        }

        ServiceClientJob(bool isAutoDelete, IConfig* config = GetDefaultConfig())
            : ServiceJob(isAutoDelete, config)
        {
        }

    protected:
        using ServiceClientJobType = ServiceClientJob<ServiceTraitsType>;

    };

    /// Defines a class that extends ServiceTraits and implements that 
    /// required static functions.
    #define AWS_FEATURE_GEM_SERVICE(SERVICE_NAME) \
        AWS_SERVICE_TRAITS_TEMPLATE(SERVICE_NAME, nullptr, nullptr) \
        using SERVICE_NAME##ServiceClientJob = AWSCore::ServiceClientJob<SERVICE_NAME##ServiceTraits>;

    #define AWS_CUSTOM_SERVICE(SERVICE_NAME, RESTAPI_ID, RESTAPI_STAGE) \
        AWS_SERVICE_TRAITS_TEMPLATE(SERVICE_NAME, RESTAPI_ID, RESTAPI_STAGE) \
        using SERVICE_NAME##ServiceClientJob = AWSCore::ServiceClientJob<SERVICE_NAME##ServiceTraits>;

} // namespace AWSCore
