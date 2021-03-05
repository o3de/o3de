/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialBuilder final
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(MaterialBuilder, "{861C0937-7671-40DC-8E44-6D432ABB9932}");

            static const char* JobKey;

            MaterialBuilder() = default;
            ~MaterialBuilder();

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

            /// Register to builder and listen to builder command
            void RegisterBuilder();

        private:
            
            bool m_isShuttingDown = false;
        };

    } // namespace RPI
} // namespace AZ