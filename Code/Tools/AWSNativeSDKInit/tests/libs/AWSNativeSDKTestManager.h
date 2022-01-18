/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/PlatformIncl.h>

#include <AWSNativeSDKInit/AWSMemoryInterface.h>

#include <aws/core/Aws.h>

namespace AWSNativeSDKTestLibs
{
    // Entry point for AWSNativeSDK's initialization and shutdown for test environment
    // Use an AZ::Environment variable to enforce only one init and shutdown
    class AWSNativeSDKTestManager
    {
    public:
        static constexpr const char SdkManagerTag[] = "TestAWSSDKManager";

        AWSNativeSDKTestManager();
        ~AWSNativeSDKTestManager();

        static void Init();
        static void Shutdown();

    private:
        static AZ::EnvironmentVariable<AWSNativeSDKTestManager> s_sdkManager;

        AWSNativeSDKInit::MemoryManager m_memoryManager;
        Aws::SDKOptions m_awsSDKOptions;
    };
}
