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
