/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AZ
{
    namespace RPI
    {
        class AnyAsset;

        class AnyAssetBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(AnyAssetBuilder, "{5D7CC67C-1AB3-4906-8311-76A7157912D3}");

            AnyAssetBuilder() = default;
            ~AnyAssetBuilder();

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override; 

            void RegisterBuilder();
            
            bool m_isShuttingDown = false;
        };

    } // namespace RPI_Builder
} // namespace AZ
