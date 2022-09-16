/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/JSON/document.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialTypePreBuilder final
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(MaterialTypePreBuilder, "{C9098D67-A075-4209-875E-A95FD887B039}");

            static const char* JobKey;

            MaterialTypePreBuilder() = default;
            ~MaterialTypePreBuilder();

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
