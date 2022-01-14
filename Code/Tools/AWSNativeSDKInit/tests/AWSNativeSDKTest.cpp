/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AWSNativeSDKTest.h>

#include <AzCore/Module/Environment.h>
#include <AzTest/Utils.h>

#include <AWSNativeSDKInit/AWSLogSystemInterface.h>
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/AWSLogging.h>

namespace AWSNativeSDKTest
{
    AZ::EnvironmentVariable<TestSDKManager> TestSDKManager::s_sdkManager = nullptr;

    TestSDKManager::TestSDKManager()
    {
        AZ::Test::SetEnv("AWS_DEFAULT_REGION", "us-east-1", 1);
        m_awsSDKOptions.memoryManagementOptions.memoryManager = &m_memoryManager;
        Aws::InitAPI(m_awsSDKOptions);
    }

    TestSDKManager::~TestSDKManager()
    {
        Aws::ShutdownAPI(m_awsSDKOptions);
        AZ::Test::UnsetEnv("AWS_DEFAULT_REGION");
    }

    void TestSDKManager::Init()
    {
        s_sdkManager = AZ::Environment::CreateVariable<TestSDKManager>(TestSDKManager::SdkManagerTag);
    }

    void TestSDKManager::Shutdown()
    {
        s_sdkManager = nullptr;
    }
}
