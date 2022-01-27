/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AWSCore_Traits_Platform.h>

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/http/Scheme.h>
#include <aws/core/Region.h>
#include <aws/core/http/HttpTypes.h>
AZ_POP_DISABLE_WARNING

#include <AWSCoreBus.h>

// Forward declarations
namespace AZ
{
    class JobContext;
}

namespace Aws 
{
    namespace Auth
    {
        class AWSCredentialsProvider;
    }
    
    namespace Utils
    {

        namespace RateLimits
        {
            class RateLimiterInterface;
        }

        namespace Threading
        {
            class Executor;
        }

    }

    namespace Client
    {
        class RetryStrategy;
        struct ClientConfiguration;
    }

    namespace Http
    {
        class HttpClient;
    }

}

namespace AWSCore
{

    /// Provides configuration for an AwsApiJob.
    class IAwsApiJobConfig
    {
    public:
        virtual ~IAwsApiJobConfig() = default;

        virtual AZ::JobContext* GetJobContext() = 0;
    };

    /// Encapsulates all the properties that can be used to configured the 
    /// operation of AWS jobs.
    class AwsApiJobConfig
        : public virtual IAwsApiJobConfig
    {

    public:
        AZ_CLASS_ALLOCATOR(AwsApiJobConfig, AZ::SystemAllocator, 0);

        using InitializerFunction = AZStd::function<void(AwsApiJobConfig& config)>;

        /// Initialize an AwsApiClientJobConfig object.
        ///
        /// \param defaultConfig - the config object that provides values when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        AwsApiJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : m_defaultConfig{ defaultConfig }
        {
            if (initializer)
            {
                initializer(*this);
            }
        }

        /// Type used to encapsulate override values.
        template<typename T>
        using Override = AZStd::optional<T>;

        // TODO: document the individual configuration settings
        Override<AZ::JobContext*> jobContext;
        Override<std::shared_ptr<Aws::Auth::AWSCredentialsProvider>> credentialsProvider;
        Override<Aws::String> userAgent;
        Override<Aws::Http::Scheme> scheme;
        Override<Aws::String> region;
        Override<unsigned> maxConnections;
        Override<long> requestTimeoutMs;
        Override<long> connectTimeoutMs;
        Override<std::shared_ptr<Aws::Client::RetryStrategy>> retryStrategy;
        Override<Aws::String> endpointOverride;
        Override<Aws::String> proxyHost;
        Override<unsigned> proxyPort;
        Override<Aws::String> proxyUserName;
        Override<Aws::String> proxyPassword;
        Override<std::shared_ptr<Aws::Utils::Threading::Executor>> executor;
        Override<bool> verifySSL;
        Override<std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface>> writeRateLimiter;
        Override<std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface>> readRateLimiter;
        Override<Aws::Http::TransferLibType> httpLibOverride;
#if AWSCORE_BACKWARD_INCOMPATIBLE_CHANGE
        Override<Aws::Client::FollowRedirectsPolicy> followRedirects;
#else
        Override<bool> followRedirects;
#endif
        Override<Aws::String> caFile;

        /// Applies settings changes made after first use.
        virtual void ApplySettings();

        //////////////////////////////////////////////////////////////////////////
        // IAwsApiJobConfig implementations
        AZ::JobContext* GetJobContext() override;

        /// Get a ClientConfiguration object initialized using the current settings.
        /// The base settings object's overrides are applied first, then this objects 
        /// overrides are applied. By default all ClientConfiguration members will 
        /// have default values (as set by the 
        Aws::Client::ClientConfiguration GetClientConfiguration() const;

    protected:
        /// Ensures that ApplySettings has been called.
        void EnsureSettingsApplied()
        {
            if (!m_settingsApplied)
            {
                ApplySettings();
            }
        }

        /// Helper function for applying Override typed members.
        template<typename T>
        static void CheckAndSet(const Override<T>& src, T& dst)
        {
            if (src.has_value())
            {
                dst = src.value();
            }
        }

        /// Call Visit for m_defaultConfig, then call visitor for this object. Is
        /// templated so that it can be used to visit derived types as 
        /// long as they define an m_defaultConfig of that type.
        template<class ConfigType>
        void Visit(AZStd::function<void(const ConfigType&)> visitor) const
        {
            if (ConfigType::m_defaultConfig)
            {
                ConfigType::m_defaultConfig->Visit(visitor);
            }
            visitor(*this);
        }

        /// Get the CredentialsProvider from this settings object, if set, or
        /// from the base settings object. By default a nullptr is returned.
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() const;

    private:
        /// The settings object with values overridden by this settings object, or
        /// nullptr if this settings object is the root.
        const AwsApiJobConfig* const m_defaultConfig;

        /// True after ApplySettings is called.
        bool m_settingsApplied{ false };

        /// Set when ApplySettings is called.
        AZ::JobContext* m_jobContext{ nullptr };

        // Copy and assignment not allowed
        AwsApiJobConfig(const AwsApiJobConfig& base) = delete;
        AwsApiJobConfig& operator=(AwsApiJobConfig& other) = delete;
    };

    template<class ConfigType>
    class AwsApiJobConfigHolder
        : protected AWSCoreNotificationsBus::Handler
    {
    public:
        ~AwsApiJobConfigHolder() override
        {
            AWSCoreNotificationsBus::Handler::BusDisconnect();
        }

        ConfigType* GetConfig(AwsApiJobConfig* defaultConfig = nullptr, typename ConfigType::InitializerFunction initializer = nullptr)
        {
            if (!m_config)
            {
                AWSCoreNotificationsBus::Handler::BusConnect();
                m_config.reset(aznew ConfigType(defaultConfig, initializer));
            }
            return m_config.get();
        }

        void OnSDKInitialized() override {}

        //! AWSCore is deactivating which allows the configuration
        //! objects to delete any cached clients or other data allocated using
        //! the AWS API's allocator.
        void OnSDKShutdownStarted() override
        {
            m_config.reset();
        }

    private:
        AZStd::unique_ptr<ConfigType> m_config;
    };

} // namespace AWSCore

