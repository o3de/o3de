/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace EMotionFX
{
    namespace EMotionFXBuilder
    {
        class AnimGraphBuilderWorker
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_RTTI(AnimGraphBuilderWorker, "{4EB80858-E7A8-46B7-8F05-B51F49050AF0}");

            void RegisterBuilderWorker();

            //!AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);

            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            bool ParseProductDependencies(const AZStd::string& fullPath, const AZStd::string& sourceFile, AZStd::vector<AssetBuilderSDK::ProductDependency>& pathDependencies);

        private:
            bool m_isShuttingDown = false;
        };
    } // namespace EMotionFXBuilder
} // namespace EMotionFX
