/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AWSNativeSDKInit/AWSMemoryInterface.h>

#include <AzCore/Module/Environment.h>

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <AzCore/PlatformIncl.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/Aws.h>
AZ_POP_DISABLE_WARNING
#endif

namespace AWSNativeSDKInit
{
    //! Entry point for Open 3D Engine managing the AWSNativeSDK's initialization and shutdown requirements
    //! Use an AZ::Environment variable to enforce only one init and shutdown
    class InitializationManager
    {
    public:
        static const char* const initializationManagerTag;

        InitializationManager();
        ~InitializationManager();

        //! Call to guarantee that the API is initialized with proper Open 3D Engine settings.
        //! It's fine to call this from every module which needs to use the NativeSDK
        //! Creates a static shared pointer using the AZ EnvironmentVariable system.
        //! This will prevent a the AWS SDK from going through the shutdown routine until all references are gone, or
        //! the AZ::EnvironmentVariable system is brought down.
        static void InitAwsApi();

        //! Returns true if the AWS SDK is initialized and ready to be used.
        static bool IsInitialized();

        //! Remove reference, if final reference then shut down SDK.
        static void Shutdown();

        //! Call this after InitAwsApi to prevent any reachout to the Amazon EC2 instance metadata service (IMDS).
        //! Unless you are running on EC2 compute this is recommended, otherwise AWS C++ SDK may try to call IMDS for region, config
        //! or credential information, which will fail on non EC2 compute and waste resources. 
        //! Note: This is a helper function for managing the environment variable, AWS_EC2_METADATA_DISABLED, but impacts just the current application's environment.
        //! @param force If true, always set AWS_EC2_METADATA_DISABLED to true, otherwise only set if environment variable is not set.
        //! @returns True if env var was set or currently prevents calls, False otherwise 
        static bool PreventAwsEC2MetadataCalls(bool force = false);

    private:    
        void InitializeAwsApiInternal();
        void ShutdownAwsApiInternal();

        MemoryManager m_memoryManager;

        static AZ::EnvironmentVariable<InitializationManager> s_initManager;
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::SDKOptions m_awsSDKOptions;
#endif
    };
}
