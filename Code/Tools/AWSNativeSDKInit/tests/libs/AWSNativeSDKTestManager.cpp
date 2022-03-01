/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AWSNativeSDKTestManager.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Utils/Utils.h>

#include <AWSNativeSDKInit/AWSLogSystemInterface.h>
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/AWSLogging.h>

namespace AWSNativeSDKTestLibs
{
    AZ::EnvironmentVariable<AWSNativeSDKTestManager> AWSNativeSDKTestManager::s_sdkManager = nullptr;

    AWSNativeSDKTestManager::AWSNativeSDKTestManager()
    {
        AZ::Utils::SetEnv("AWS_DEFAULT_REGION", "us-east-1", 1);
        m_awsSDKOptions.memoryManagementOptions.memoryManager = &m_memoryManager;
        Aws::InitAPI(m_awsSDKOptions);
    }

    AWSNativeSDKTestManager::~AWSNativeSDKTestManager()
    {
        Aws::ShutdownAPI(m_awsSDKOptions);
        AZ::Utils::UnsetEnv("AWS_DEFAULT_REGION");
    }

    void AWSNativeSDKTestManager::Init()
    {
        s_sdkManager = AZ::Environment::CreateVariable<AWSNativeSDKTestManager>(AWSNativeSDKTestManager::SdkManagerTag);
    }

    void AWSNativeSDKTestManager::Shutdown()
    {
        s_sdkManager = nullptr;
    }
}
