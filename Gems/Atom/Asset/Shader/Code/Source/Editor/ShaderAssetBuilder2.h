/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
