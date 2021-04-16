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

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairAssetBuilder final
                : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
            {
            public:
                AZ_RTTI(AnimGraphBuilderWorker, "{7D77A133-115E-4A14-860D-C1DB9422C190}");

                void RegisterBuilder();

                //! AssetBuilderSDK::AssetBuilderCommandBus interface
                void ShutDown() override;

                void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);

                void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            private:
                bool m_isShuttingDown = false;
            };
        }
    } // namespace Render
} // namespace AZ
