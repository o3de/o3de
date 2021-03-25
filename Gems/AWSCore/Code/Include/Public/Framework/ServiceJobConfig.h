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

#include <Framework/HttpRequestJobConfig.h>

namespace AWSCore
{

    /// Provides configuration needed by service jobs.
    class IServiceJobConfig
        : public virtual IHttpRequestJobConfig
    {
    };

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4250 )
    // warning C4250: 'AWSCore::ServiceJobConfig' : inherits 'AWSCore::AwsApiJobConfig::AWSCore::AwsApiJobConfig::GetJobContext' via dominance
    // Thanks to http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors for the explanation
    // This is the expected and desired behavior. The warning is superfluous.

#endif

/// Provides service job configuration using settings properties.
    class ServiceJobConfig
        : public HttpRequestJobConfig
        , public virtual IServiceJobConfig
    {

    public:

        AZ_CLASS_ALLOCATOR(ServiceJobConfig, AZ::SystemAllocator, 0);

        using InitializerFunction = AZStd::function<void(ServiceJobConfig& config)>;

        /// Initialize an ServiceJobConfig object.
        ///
        /// \param defaultConfig - the config object that provides values when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        ServiceJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : HttpRequestJobConfig{ defaultConfig }
        {
            if (initializer)
            {
                initializer(*this);
            }
        }

        void ApplySettings() override;

    private:

    };

#ifdef _MSC_VER 
#pragma warning( pop ) // C4250
#endif

} // namespace AWSCore
