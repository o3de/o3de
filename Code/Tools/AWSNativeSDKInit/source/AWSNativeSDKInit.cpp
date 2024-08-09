/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AWSNativeSDKInit/AWSNativeSDKInit.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Utils/Utils.h>

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <AWSNativeSDKInit/AWSLogSystemInterface.h>
#include <aws/core/Aws.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#endif

namespace AWSNativeSDKInit
{
    static constexpr char AWS_EC2_METADATA_DISABLED[] = "AWS_EC2_METADATA_DISABLED";

    namespace Platform
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        void CustomizeSDKOptions(Aws::SDKOptions& options);
        void CustomizeShutdown();

        void CopyCaCertBundle();
#endif
    } // namespace Platform

    const char* const InitializationManager::initializationManagerTag = "AWSNativeSDKInitializer";
    AZ::EnvironmentVariable<InitializationManager> InitializationManager::s_initManager = nullptr;

    InitializationManager::InitializationManager()
    {
        InitializeAwsApiInternal();
    }

    InitializationManager::~InitializationManager()
    {
        ShutdownAwsApiInternal();
    }

    void InitializationManager::InitAwsApi()
    {
        s_initManager = AZ::Environment::CreateVariable<InitializationManager>(initializationManagerTag);

        Platform::CopyCaCertBundle();
    }

    void InitializationManager::Shutdown()
    {
        s_initManager = nullptr;
    }

    bool InitializationManager::IsInitialized()
    {
        return s_initManager.IsConstructed();
    }

    void InitializationManager::InitializeAwsApiInternal()
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::Utils::Logging::LogLevel logLevel;
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        logLevel = Aws::Utils::Logging::LogLevel::Warn;
#else
        logLevel = Aws::Utils::Logging::LogLevel::Error;
#endif
        m_awsSDKOptions.loggingOptions.logLevel = logLevel;
        m_awsSDKOptions.loggingOptions.logger_create_fn = [logLevel]()
        {
            return Aws::MakeShared<AWSLogSystemInterface>("AWS", logLevel);
        };

        m_awsSDKOptions.memoryManagementOptions.memoryManager = &m_memoryManager;
        Platform::CustomizeSDKOptions(m_awsSDKOptions);
        Aws::InitAPI(m_awsSDKOptions);

#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    }

    bool InitializationManager::PreventAwsEC2MetadataCalls(bool force)
    {
        bool prevented = false;
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)

        // Helper function to prevent calls to the Amazon EC2 instance metadata service (IMDS), unless environment var has been
        // configured. AWS C++ SDK may reach out to EC2 IMDS for region, config or credentials, but unless code is running on EC2
        // compute such calls will fail and waste network resources. Note: AWS C++ SDK explicitly only checks if lower case version of
        // AWS_EC2_METADATA_DISABLED == "true", otherwise it will enable the EC2 metadata service calls.
        const auto ec2MetadataEnvVar = Aws::Environment::GetEnv(AWS_EC2_METADATA_DISABLED);
        if (ec2MetadataEnvVar.empty() || force)
        {
            prevented = true;
            AZ::Utils::SetEnv(AWS_EC2_METADATA_DISABLED, "true", true);
        }
        else if (Aws::Utils::StringUtils::ToLower(ec2MetadataEnvVar.c_str()) == "true")
        {
            prevented = true;
        }

#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        return prevented;
    }

    void InitializationManager::ShutdownAwsApiInternal()
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::ShutdownAPI(m_awsSDKOptions);
        Platform::CustomizeShutdown();
#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    }

} // namespace AWSNativeSDKInit
