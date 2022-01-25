/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
