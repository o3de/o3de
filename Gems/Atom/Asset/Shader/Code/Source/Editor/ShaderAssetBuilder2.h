/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }

    namespace RHI
    {
        class ShaderPlatformInterface;
    }

    namespace ShaderBuilder
    {
        struct AzslData;

        class ShaderAssetBuilder2
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(ShaderAssetBuilder2, "{C94DA151-82BC-4475-86FA-E6C92A0BD6F8}");

            static constexpr const char* ShaderAssetBuilder2JobKey = "Shader Asset 2";

            ShaderAssetBuilder2() = default;
            ~ShaderAssetBuilder2() = default;

            // Asset Builder Callback Functions ...
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface overrides ...
            void ShutDown() override { };

        private:
            AZ_DISABLE_COPY_MOVE(ShaderAssetBuilder2);
        };

    } // ShaderBuilder
} // AZ
