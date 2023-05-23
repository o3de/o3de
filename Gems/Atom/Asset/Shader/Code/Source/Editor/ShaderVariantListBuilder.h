/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    namespace ShaderBuilder
    {
        // The ShaderVariantListBuilder produces two kinds of Intermediate Assets from a single
        // .shadervariantlist file.
        // The first intermediate asset is the ".hashedvariantlist" file:
        //     Will contain a consolidated list of all variants that will be necessary to create a ShaderVariantTreeAsset.
        //     The ShaderVariantAssetBuilder will construct a single product ShaderVariantTreeAsset for each ".hashedvariantlist" file.
        // The second intermediate asset is the ".hashedvariantinfo" files:
        //     There will be one of these for each variant specified in the .shadervariantlist file.
        //     These files will be named as <Shader Name>_<StableId>.hashedvariantinfo.
        //     The ShaderVariantAssetBuilder will construct a single product (.azshadervariant) ShaderVariantAsset for each
        //     ".hashedvariantinfo" file ( and for each RHI). For example, the Windows platform supports both DX12 and Vulkan,
        //     For for each <Shader Name>_<StableId>.hashedvariantinfo file, there will be the following products (for each Supervariant
        //     specified in the corresponding .shader file)
        //         - <Shader Name><Supervariant Name>_dx12_<StableId>.azshadervariant
        //         - <Shader Name><Supervariant Name>_vulkan_<StableId>.azshadervariant
        class ShaderVariantListBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO_WITH_NAME_DECL(ShaderVariantListBuilder);

            static constexpr char JobKey[] = "HashedShaderVariantList";

            ShaderVariantListBuilder() = default;
            ~ShaderVariantListBuilder() = default;

            // Asset Builder Callback Functions ...
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface overrides ...
            void ShutDown() override;

        private:
            AZ_DISABLE_COPY_MOVE(ShaderVariantListBuilder);

            // Keys of job parameters that are shared between CreateJobs() and ProcessJob().
            static constexpr uint32_t ShaderVariantLoadErrorParam = 0;
            static constexpr uint32_t ShouldExitEarlyFromProcessJobParam = 1;
            static constexpr uint32_t ShaderVariantListAbsolutePathJobParam = 2;
            static constexpr uint32_t ShaderAbsolutePathJobParam = 3;

        };

    } // ShaderBuilder
} // AZ
