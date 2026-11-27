/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace OpenXRVkBuilders
{
    class OpenXRAssetsBuilder final
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_TYPE_INFO(OpenXRAssetsBuilder, "{1D053000-7799-459D-B99B-FF6AE6394BC1}");

        static constexpr char LogName[] = "OpenXRAssetsBuilder";

        static constexpr const char* InteractionProfilesAssetJobKey = "XR Interaction Profiles Asset";
        static constexpr const char* ActionSetsAssetJobKey = "XR Action Sets Asset";
    
        OpenXRAssetsBuilder() = default;
        ~OpenXRAssetsBuilder() = default;
    
        // Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;
    
        // AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override { };
  
    private:
        AZ_DISABLE_COPY_MOVE(OpenXRAssetsBuilder);

        void CreateInteractionProfilesAssetJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessInteractionProfilesAssetJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        void CreateActionSetsAssetJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessActionSetsAssetJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;
    };

} // namespace OpenXRVkBuilders
