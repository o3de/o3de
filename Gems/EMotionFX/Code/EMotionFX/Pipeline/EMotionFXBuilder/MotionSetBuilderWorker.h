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
        class MotionSetBuilderWorker
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_RTTI(MotionSetBuilderWorker, "{7C70FBB0-79A4-4288-A989-A5DA6D05802F}");

            void RegisterBuilderWorker();

            //!AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);

            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            bool ParseProductDependencies(const AZStd::string& fullPath, const AZStd::string& sourceFile, AssetBuilderSDK::ProductPathDependencySet& pathDependencies);

        private:
            bool m_isShuttingDown = false;
        };
    }
}
