/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AWSNativeSDKInit/AWSNativeSDKInit.h>

#include <AzCore/Module/Environment.h>

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <AWSNativeSDKInit/AWSLogSystemInterface.h>
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#endif

namespace AWSNativeSDKInit
{
    namespace Platform
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        void CustomizeSDKOptions(Aws::SDKOptions& options);
        void CustomizeShutdown();

        void CopyCaCertBundle();
#endif
    }

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

    void InitializationManager::ShutdownAwsApiInternal()
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::ShutdownAPI(m_awsSDKOptions);
        Platform::CustomizeShutdown();
#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    }

}
