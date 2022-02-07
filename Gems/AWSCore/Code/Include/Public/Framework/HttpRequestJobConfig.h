/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/AWSApiJobConfig.h>

namespace AWSCore
{

    //! Provides configuration needed by HTTP request jobs.
    class IHttpRequestJobConfig
        : public virtual IAwsApiJobConfig
    {

    public:

        virtual std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> GetReadRateLimiter() = 0;
        virtual std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> GetWriteRateLimiter() = 0;
        virtual std::shared_ptr<Aws::Http::HttpClient> GetHttpClient() = 0;
        virtual const Aws::String& GetUserAgent() = 0;

    };

    // warning C4250: 'AWSCore::HttpRequestJobConfig' : inherits 'AWSCore::AwsApiJobConfig::AWSCore::AwsApiJobConfig::GetJobContext' via dominance
    // Thanks to http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors for the explanation
    // This is the expected and desired behavior. The warning is superfluous.
    AZ_PUSH_DISABLE_WARNING(4250, "-Wunknown-warning-option")
    //! Provides service job configuration using settings properties.
    class HttpRequestJobConfig
        : public AwsApiJobConfig
        , public virtual IHttpRequestJobConfig
    {

    public:

        AZ_CLASS_ALLOCATOR(HttpRequestJobConfig, AZ::SystemAllocator, 0);

        using InitializerFunction = AZStd::function<void(HttpRequestJobConfig& config)>;

        /// Initialize an HttpRequestJobConfig object.
        ///
        /// \param defaultConfig - the config object that provides values when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        HttpRequestJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : AwsApiJobConfig{ defaultConfig }
        {
            if (initializer)
            {
                initializer(*this);
            }
        }

        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> GetReadRateLimiter() override
        {
            EnsureSettingsApplied();
            return m_readRateLimiter;
        }

        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> GetWriteRateLimiter() override
        {
            EnsureSettingsApplied();
            return m_writeRateLimiter;
        }

        std::shared_ptr<Aws::Http::HttpClient> GetHttpClient() override
        {
            EnsureSettingsApplied();
            return m_httpClient;
        }

        const Aws::String& GetUserAgent() override
        {
            EnsureSettingsApplied();
            return m_userAgent;
        }

        void ApplySettings() override;

    private:
        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_readRateLimiter{ nullptr };
        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_writeRateLimiter{ nullptr };
        std::shared_ptr<Aws::Http::HttpClient> m_httpClient{ nullptr };
        Aws::String m_userAgent{};
    };
    AZ_POP_DISABLE_WARNING

} // namespace AWSCore
