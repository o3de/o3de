/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace MiniAudio
{
    class SoundAssetBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(SoundAssetBuilder, "{b7db2037-18c7-4bc7-9434-7cd5523d6649}");

        SoundAssetBuilder() = default;

        //! AssetBuilderCommandBus overrides ...
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;
        void ShutDown() override {}
    };

} // namespace MiniAudio
