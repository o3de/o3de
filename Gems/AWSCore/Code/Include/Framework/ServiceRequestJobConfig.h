/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/ServiceClientJobConfig.h>

namespace AWSCore
{

    class IServiceRequestJobConfig
        : public virtual IServiceClientJobConfig
    {

    public:

        virtual const Aws::String& GetRequestUrl() = 0;
        virtual std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() = 0;

        virtual bool IsValid() const = 0;
    };

    // warning C4250: 'AWSCore::ServiceRequestJobConfig<RequestType>' : inherits 'AWSCore::AwsApiJobConfig::AWSCore::AwsApiJobConfig::GetJobContext' via dominance
    // Thanks to http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors for the explanation
    // This is the expected and desired behavior. The warning is superfluous.
    AZ_PUSH_DISABLE_WARNING(4250, "-Wunknown-warning-option")
    template<class RequestType>
    class ServiceRequestJobConfig
        : public ServiceClientJobConfig<typename RequestType::ServiceTraits>
        , public virtual IServiceRequestJobConfig
    {

    public:

        AZ_CLASS_ALLOCATOR(ServiceRequestJobConfig, AZ::SystemAllocator, 0);

        using InitializerFunction = AZStd::function<void(ServiceClientJobConfig<typename RequestType::ServiceTraits>& config)>;
        using ServiceClientJobConfigType = ServiceClientJobConfig<typename RequestType::ServiceTraits>;

        /// Initialize an ServiceRequestJobConfig object.
        ///
        /// \param defaultConfig - the config object that provides values when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        ServiceRequestJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : ServiceClientJobConfigType{ defaultConfig }
        {
            if (initializer)
            {
                initializer(*this);
            }
        }

        const Aws::String& GetRequestUrl() override
        {
            ServiceClientJobConfigType::EnsureSettingsApplied();
            return m_requestUrl;
        }

        bool IsValid() const override
        {
            // If we failed to get mappings we'll have no URL and should not try to make a request
            return (m_requestUrl.length() > 0);
        }

        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override
        {
            ServiceClientJobConfigType::EnsureSettingsApplied();
            return m_credentialsProvider;
        }

        void ApplySettings() override
        {
            ServiceClientJobConfigType::ApplySettings();

            m_requestUrl = GetServiceUrl().c_str();
            if (m_requestUrl.length())
            {
                m_requestUrl += RequestType::Path();
            }

            m_credentialsProvider = AwsApiJobConfig::GetCredentialsProvider();
        }

    private:

        Aws::String m_requestUrl;
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_credentialsProvider;

    };
    AZ_POP_DISABLE_WARNING

} // namespace AWSCore
