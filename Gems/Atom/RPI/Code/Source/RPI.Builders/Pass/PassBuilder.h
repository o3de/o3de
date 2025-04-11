/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace AZ
{
    namespace RPI
    {
        class PassBuilder final
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(PassBuilder, "{3F33C35C-607C-4D39-87C4-3614D88A75CD}");

            static const char* JobKey;

            PassBuilder() = default;
            ~PassBuilder();

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
