/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzFramework/Asset/Benchmark/BenchmarkSettingsAsset.h>
#include <AzFramework/Asset/Benchmark/BenchmarkAsset.h>

namespace BenchmarkAssetBuilder
{
    //! This "builds" the BenchmarkSettingsAsset asset by generating a series of BenchmarkAsset outputs based on the settings.
    class BenchmarkAssetBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler // this will deliver you the "shut down!" message on another thread.
    {
    public:
        AZ_RTTI(BenchmarkAssetBuilderWorker, "{30ADD4F0-D582-47E5-9E79-C71A88127872}");

        BenchmarkAssetBuilderWorker() = default;
        ~BenchmarkAssetBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

    private:
        bool m_isShuttingDown = false;

        //! Provide some simple validation to ensure that our settings won't do anything horrible on generation.
        bool ValidateSettings(const AZStd::unique_ptr<AzFramework::BenchmarkSettingsAsset>& settingsPtr);

        //! Trivial helper method to convert from a concrete BenchmarkSettingsAsset to job parameter strings.
        void ConvertSettingsToJobParameters(const AzFramework::BenchmarkSettingsAsset& settings,
            AssetBuilderSDK::JobParameterMap& jobParameters);
        //! Trivial helper method to convert from job parameter strings to a concrete BenchmarkSettingsAsset.
        void ConvertJobParametersToSettings(const AssetBuilderSDK::JobParameterMap& jobParameters,
            AzFramework::BenchmarkSettingsAsset& settings);

        //! Fill a buffer with deterministically random numbers.
        void FillBuffer(AZStd::vector<uint8_t>& buffer, uint64_t bufferSize, uint64_t seed);

        //! Recursively generate an asset and all assets that it depends on in the generated tree.
        AssetBuilderSDK::ProcessJobResultCode GenerateDependentAssetSubTree(
            const AzFramework::BenchmarkSettingsAsset& settings,
            AZ::Uuid sourceUuid,
            const AZStd::string& sourceAssetPath,
            const AZStd::string& generatedAssetPath,
            uint32_t curDepth,
            uint32_t& uniqueSubId,
            AssetBuilderSDK::ProcessJobResponse& response);
    };
}
