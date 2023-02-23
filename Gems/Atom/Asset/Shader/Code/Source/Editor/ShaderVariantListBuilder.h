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
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

#include "ShaderBuilderUtility.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        struct AzslData; 

        class ShaderVariantListBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO_WITH_NAME_DECL(ShaderVariantListBuilder);

            // TODO. Because we are merging this code in phases, we start with the "shadervariantlist2"
            // extension which is the same "shadervariantlist" file format the we all know and love.
            // Later we'll do another PR that actually referenced the "shadervariantlist" extension.
            static constexpr char Extension[] = "shadervariantlist2";

            static constexpr char JobKey[] = "HashedShaderVariantList";

            // Setting this to false will always rebuild all variants when a .shadervariantlist changes.
            // Enabled by default.
            static constexpr char EnableHashCompareRegistryKey[] = "/O3DE/Atom/Shaders/ShaderVariantListBuilder/EnableHashCompare";
            static constexpr bool EnableHashCompareRegistryDefaultValue = true;

            // If a .shadervariantlist changes several times within this time period,
            // the IsNew status of each previous variant in .shadervariantlist will be preserved.
            static constexpr char SuddenChangeInMinutesRegistryKey[] = "/O3DE/Atom/Shaders/ShaderVariantListBuilder/SuddenChangeInMinutes";
            static constexpr unsigned SuddenChangeInMinutesRegistryDefaultValue = 5;

            ShaderVariantListBuilder() = default;
            ~ShaderVariantListBuilder() = default;

            // Asset Builder Callback Functions ...
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface overrides ...
            void ShutDown() override { };

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
