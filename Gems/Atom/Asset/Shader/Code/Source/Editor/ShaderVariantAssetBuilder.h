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
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

#include "ShaderBuilderUtility.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        struct AzslData;

        //! This is nothing more than a class to help consolidate all
        //! the data needed to generate a shader variant and prevent
        //! all the functions involved in the process to have too many
        //! arguments. 
        struct ShaderVariantCreationContext
        {
            RHI::ShaderPlatformInterface& m_shaderPlatformInterface;
            const AssetBuilderSDK::PlatformInfo& m_platformInfo;
            const RHI::ShaderBuildArguments& m_shaderBuildArguments;
            //! Used to write temporary files during shader compilation, like *.hlsl, or *.air, or *.metallib, etc.
            const AZStd::string& m_tempDirPath;
            //! Used to synchronize versions of the ShaderAsset and ShaderVariantAsset,
            //! especially during hot-reload. A (ShaderVariantAsset.timestamp) >= (ShaderAsset.timestamp).
            const AZ::u64 m_assetBuildTimestamp;
            const RPI::ShaderSourceData& m_shaderSourceDataDescriptor;
            const RPI::ShaderOptionGroupLayout& m_shaderOptionGroupLayout;
            const MapOfStringToStageType& m_shaderEntryPoints;
            const Data::AssetId m_shaderVariantAssetId;
            const AZStd::string& m_shaderStemNamePrefix; //<shaderName>-<supervariantName>
            const AZStd::string& m_hlslSourcePath;
            const AZStd::string& m_hlslSourceContent;
        };

        class ShaderVariantAssetBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(ShaderVariantAssetBuilder, "{C959AEC2-2083-4488-AD88-F61B1144535B}");

            static constexpr char ShaderVariantAssetBuilderJobKey[] = "Shader Variant Asset";

            ShaderVariantAssetBuilder() = default;
            ~ShaderVariantAssetBuilder() = default;

            // Asset Builder Callback Functions ...
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            //! The ShaderVariantAsset returned by this function won't be written to the filesystem.
            //! You should call SerializeOutShaderVariantAsset to write it to the temp folder assigned
            //! by the asset processor.
            static AZ::Outcome<Data::Asset<RPI::ShaderVariantAsset>, AZStd::string> CreateShaderVariantAsset(
                const RPI::ShaderVariantListSourceData::VariantInfo& shaderVariantInfo,
                ShaderVariantCreationContext& creationContext,
                AZStd::optional<RHI::ShaderPlatformInterface::ByProducts>& outputByproducts);

            static bool SerializeOutShaderVariantAsset(
                const Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset,
                const AZStd::string& shaderStemNamePrefix, const AZStd::string& tempDirPath,
                const RHI::ShaderPlatformInterface& shaderPlatformInterface, const uint32_t productSubID, AssetBuilderSDK::JobProduct& assetProduct);

            // AssetBuilderSDK::AssetBuilderCommandBus interface overrides ...
            void ShutDown() override { };

        private:
            AZ_DISABLE_COPY_MOVE(ShaderVariantAssetBuilder);

            static constexpr uint32_t ShaderVariantLoadErrorParam = 0;
            static constexpr uint32_t ShaderSourceFilePathJobParam = 2;
            static constexpr uint32_t ShaderVariantJobVariantParam = 3;
            static constexpr uint32_t ShouldExitEarlyFromProcessJobParam = 4;

            //! Called from ProcessJob when the job is supposed to create a ShaderVariantTreeAsset.
            void ProcessShaderVariantTreeJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            //! Called from ProcessJob when the job is supposed to create ShaderVariantAssets. One ShaderVariantAsset will be produced per RHI::APIType
            //! supported by the platform.
            void ProcessShaderVariantJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            static AZStd::string GetShaderVariantTreeAssetJobKey() { return AZStd::string::format("%s_varianttree", ShaderVariantAssetBuilderJobKey); }
            static AZStd::string GetShaderVariantAssetJobKey(RPI::ShaderVariantStableId variantStableId) { return AZStd::string::format("%s_variant_%u", ShaderVariantAssetBuilderJobKey, variantStableId.GetIndex()); }

        };

    } // ShaderBuilder
} // AZ
