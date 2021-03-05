/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
