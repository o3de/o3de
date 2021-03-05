/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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