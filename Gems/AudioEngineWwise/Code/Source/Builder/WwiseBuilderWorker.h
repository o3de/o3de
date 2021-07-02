/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
