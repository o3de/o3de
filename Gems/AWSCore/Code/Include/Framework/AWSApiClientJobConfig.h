/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/make_shared.h>
#include <Framework/AWSApiJobConfig.h>
#include <aws/s3/S3Client.h>

// Forward declarations
namespace Aws
{

    namespace Client
    {
        struct ClientConfiguration;
    }

    namespace Http
    {
        class HttpClient;
    }
    namespace Auth
    {
        class AWSCredentials;
    }

}

namespace AWSCore
{

    /// Provides configuration for AWS jobs using a specific client type.
    template<class ClientType>
    class IAwsApiClientJobConfig
        : public virtual IAwsApiJobConfig
    {

    public:
        //! Returns the created AWS client for the Job
        virtual std::shared_ptr<ClientType> GetClient() = 0;
    };

    // warning C4250: 'AWSCore::AwsApiClientJobConfig<ClientType>': inherits 'AWSCore::AwsApiJobConfig::AWSCore::AwsApiJobConfig::GetJobContext' via dominance
    // Thanks to http://stackoverflow.com/questions/11965596/diamond-inheritance-scenario-compiles-fine-in-g-but-produces-warnings-errors for the explanation
    // This is the expected and desired behavior. The warning is superfluous.
    AZ_PUSH_DISABLE_WARNING(4250, "-Wunknown-warning-option")
    /// Configuration for AWS jobs using a specific client type.
    template<class ClientType>
    class AwsApiClientJobConfig
        : public AwsApiJobConfig
        , public virtual IAwsApiClientJobConfig<ClientType>
    {

    public:
        AZ_CLASS_ALLOCATOR(AwsApiClientJobConfig, AZ::SystemAllocator);

        using AwsApiClientJobConfigType = AwsApiClientJobConfig<ClientType>;
        using InitializerFunction = AZStd::function<void(AwsApiClientJobConfigType& config)>;

        /// Initialize an AwsApiClientJobConfig object.
        ///
        /// \param defaultConfig - the config object that provides values when
        /// no override has been set in this object. The default is nullptr, which
        /// will cause a default value to be used.
        ///
        /// \param initializer - a function called to initialize this object.
        /// This simplifies the initialization of static instances. The default
        /// value is nullptr, in which case no initializer will be called.
        AwsApiClientJobConfig(AwsApiJobConfig* defaultConfig = nullptr, InitializerFunction initializer = nullptr)
            : AwsApiJobConfig{ defaultConfig }
        {
            if (initializer)
            {
                initializer(*this);
            }
        }

        ~AwsApiClientJobConfig() override = default;

        /// Gets a client initialized used currently applied settings. If
        /// any settings change after first use, code must call
        /// ApplySettings before those changes will take effect.
        std::shared_ptr<ClientType> GetClient() override
        {
            EnsureSettingsApplied();
            return m_client;
        }

    protected:
        void ApplySettings() override
        {
            AwsApiJobConfig::ApplySettings();
            m_client = CreateClient();
        }


        /// Create a client configured using this object's settings. ClientType
        /// can be any of the AWS API service clients (e.g. LambdaClient, etc.).
        std::shared_ptr<ClientType> CreateClient() const;


    private:
        /// Set by ApplySettings
        std::shared_ptr<ClientType> m_client;
    };

    template<class ClientType>
    std::shared_ptr<ClientType> AwsApiClientJobConfig<ClientType>::CreateClient() const
    {
        auto provider = GetCredentialsProvider();
        if (provider != nullptr)
        {
            // Note: This constructor for AWS Client is marked for depreciation
            return std::make_shared<ClientType>(provider, GetClientConfiguration());
        }
        else
        {
            // If no explicit credentials are provided then AWS C++ SDK will perform standard search
            // Note: This constructor for AWS Client is marked for depreciation
            return std::make_shared<ClientType>(Aws::Auth::AWSCredentials(), GetClientConfiguration());
        }
    }

    template<>
    inline std::shared_ptr<Aws::S3::S3Client> AwsApiClientJobConfig<Aws::S3::S3Client>::CreateClient() const
    {
        auto signPayloads = Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
        bool useVirtualAddressing = false;

        auto provider = AwsApiJobConfig::GetCredentialsProvider();
        if (provider != nullptr)
        {
            // Note: This constructor for AWS Client is marked for deprecation
            return std::make_shared<Aws::S3::S3Client>(provider, GetClientConfiguration(), signPayloads, useVirtualAddressing);
        }
        else
        {
            // If no explicit credentials are provided then AWS C++ SDK will perform standard search
            // Note: This constructor for AWS Client is marked for deprecation
            return std::make_shared<Aws::S3::S3Client>(
                Aws::Auth::AWSCredentials(), GetClientConfiguration(), signPayloads, useVirtualAddressing);
        }
    }
    AZ_POP_DISABLE_WARNING

} // namespace AWSCore
