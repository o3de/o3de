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
