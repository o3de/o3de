/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <Engine/Config_wwise.h>

namespace WwiseBuilder
{
    //! Wwise Builder is responsible for processing encoded audio media such as sound banks
    class WwiseBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(WwiseBuilderWorker, "{85224E40-9211-4C05-9397-06E056470171}");

        WwiseBuilderWorker();
        ~WwiseBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //! AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        AZ::Outcome<AZStd::string, AZStd::string> GatherProductDependencies(const AZStd::string& fullPath, const AZStd::string& relativePath, AssetBuilderSDK::ProductPathDependencySet& dependencies);

    private:
        void Initialize();

        AZStd::atomic_bool m_isShuttingDown;
        bool m_initialized = false;
        Audio::Wwise::ConfigurationSettings m_wwiseConfig;
    };

} // WwiseBuilder
