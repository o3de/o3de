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

#include <Atom/RPI.Edit/ResourcePool/ResourcePoolSourceData.h>

namespace AZ
{
    namespace RPI
    {
        class ResourcePoolBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(ResourcePoolBuilder, "{5F8B71F1-9D4C-49DD-9F3C-8C92CBF0600C}");

            ResourcePoolBuilder() = default;
            ~ResourcePoolBuilder();

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override; 

            /// Register builder which using this worker to process AP jobs
            void RegisterBuilder();

            /// Convert pool source data to runtime image pool asset or buffer pool asset 
            static Data::Asset<Data::AssetData> CreatePoolAssetFromSource(const ResourcePoolSourceData& sourceData);

            /// Convert resource pool source data to runtime streaming image pool asset  
            static Data::Asset<Data::AssetData> CreateStreamingPoolAssetFromSource(const ResourcePoolSourceData& sourceData);

            bool m_isShuttingDown = false;
        };

    } // namespace RPI_Builder
} // namespace AZ
