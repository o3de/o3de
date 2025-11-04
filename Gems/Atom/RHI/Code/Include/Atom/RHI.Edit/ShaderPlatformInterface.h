/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>

#include <Atom/RHI.Edit/ShaderBuildArguments.h>

namespace AssetBuilderSDK
{
    struct PlatformInfo;
}

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace AZ::RHI
{
    class ShaderResourceGroupLayout;
    class ShaderStageFunction;

    // [GFX TODO] ATOM-1668 This enum is a temporary copy of the RPI::ShaderStageType.
    // We need to decide if virtual stages are a good design for the RHI and expose one
    // unique shader stage enum that the RHI and RPI can use.
    enum ShaderHardwareStage : uint32_t
    {
        Invalid = static_cast<uint32_t>(-1),
        Vertex = 0,
        Geometry,
        Fragment,
        Compute,
        RayTracing,
    };

    //! This class provides a platform agnostic interface for the creation
    //! and manipulation of platform shader objects.
    //! WARNING: The ShaderPlatformInterface objects are singletons and will be used to process multiple shader compilation jobs.
    //! Do not store per-job configuration data in any ShaderPlatformInterface classes, as it may get stomped. Instead, pass
    //! any per-job configuration on the call stack.
    class ShaderPlatformInterface
    {
    public:
        //! This struct describes layout information of a Shader Resource Group
        //! that is part of a Pipeline.
        struct ShaderResourceGroupInfo
        {
            /// Layout of the Shader Resource Group.
            const RHI::ShaderResourceGroupLayout* m_layout;

            RHI::ShaderResourceGroupBindingInfo m_bindingInfo;
        };

        //! This struct describes binding information about root constants
        //! that is part of a Pipeline.
        struct RootConstantsInfo
        {
            /// The space id used by the constant buffer that contains the inline constants.
            uint32_t m_spaceId = ~0u;
            /// The register id used by the constant buffer that contains the inline constants.
            uint32_t m_registerId = ~0u;
            /// The total size in bytes of all inline constants.
            uint32_t m_totalSizeInBytes = 0;
        };

        using ShaderResourceGroupInfoList = AZStd::fixed_vector<ShaderResourceGroupInfo, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;

        struct ByProducts
        {
            AZStd::set<AZStd::string> m_intermediatePaths;  //!< intermediate file paths (like dxil text form)
            static constexpr uint32_t UnknownDynamicBranchCount = std::numeric_limits<uint32_t>::max();
            uint32_t m_dynamicBranchCount = UnknownDynamicBranchCount;
        };

        //! Struct used to return data when compiling the AZSL shader to the appropriate platform.
        struct StageDescriptor
        {
            ShaderHardwareStage m_stageType = ShaderHardwareStage::Invalid;
            AZStd::vector<uint8_t> m_byteCode;
            AZStd::vector<char> m_sourceCode;
            AZStd::string m_entryFunctionName;

            ByProducts m_byProducts;  //!< Optional; used for debug information
            AZStd::string m_extraData; //!< Optional; extra data that can be pass for creating the Stage function.
        };

        //! @apiUniqueIndex See GetApiUniqueIndex() for details.
        explicit ShaderPlatformInterface(uint32_t apiUniqueIndex) : m_apiUniqueIndex(apiUniqueIndex) {}

        virtual ~ShaderPlatformInterface() = default;

        //! Returns the RHI API Type that this ShaderPlatformInterface supports.
        virtual RHI::APIType GetAPIType() const = 0;

        //! Returns the RHI API Name that this ShaderPlatformInterface supports.
        virtual AZ::Name GetAPIName() const = 0;

        //! Creates the platform specific pipeline layout descriptor.
        virtual RHI::Ptr<RHI::PipelineLayoutDescriptor> CreatePipelineLayoutDescriptor() = 0;

        virtual RHI::Ptr<RHI::ShaderStageFunction> CreateShaderStageFunction(const StageDescriptor& stageDescriptor) = 0;

        virtual bool IsShaderStageForRaster(ShaderHardwareStage shaderStageType) const = 0;
        virtual bool IsShaderStageForCompute(ShaderHardwareStage shaderStageType) const = 0;
        virtual bool IsShaderStageForRayTracing(ShaderHardwareStage shaderStageType) const = 0;

        //! Compiles AZSL shader to the appropriate platform.
        virtual bool CompilePlatformInternal(
            const AssetBuilderSDK::PlatformInfo& platform,
            const AZStd::string& shaderSource,
            const AZStd::string& functionName,
            ShaderHardwareStage shaderStage,
            const AZStd::string& tempFolderPath,
            StageDescriptor& outputDescriptor,
            const RHI::ShaderBuildArguments& shaderBuildArguments,
            const bool useSpecializationConstants) const = 0;

        //! Query whether the shaders are set to build with debug information
        virtual bool BuildHasDebugInfo(const RHI::ShaderBuildArguments& shaderBuildArguments) const
        {
            return shaderBuildArguments.m_generateDebugInfo;
        }

        //! Get the filename of include file to prefix shader programs with
        virtual const char* GetAzslHeader(const AssetBuilderSDK::PlatformInfo& platform) const = 0;

        //! Builds additional platform specific data to the pipeline layout descriptor.
        //! Will be called before CompilePlatformInternal().
        virtual bool BuildPipelineLayoutDescriptor(
            RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor,
            const ShaderResourceGroupInfoList& srgInfoList,
            const RootConstantsInfo& rootConstantsInfo,
            const RHI::ShaderBuildArguments& shaderBuildArguments) = 0;
            
        //! In general, shader compilation doesn't require SRG Layout data, but RHIs like
        //! Metal don't do well if unused resources (descriptors) are not bound. If this function returns TRUE
        //! the ShaderVariantAssetBuilder will invoke BuildPipelineLayoutDescriptor() so the RHI gets the chance to
        //! build SRG Layout data which will be useful when compiling MetalISL to Metal byte code.
        virtual bool VariantCompilationRequiresSrgLayoutData() const { return false; }

        //! See AZ::RHI::Factory::GetAPIUniqueIndex() for details.
        //! See AZ::RHI::Limits::APIType::PerPlatformApiUniqueIndexMax.
        uint32_t GetAPIUniqueIndex() const { return m_apiUniqueIndex; }

    private:
        ShaderPlatformInterface() = delete;

        //! WARNING: The ShaderPlatformInterface objects are singletons and will be used to process multiple shader compilation jobs.
        //! Do not store per-job configuration data in any ShaderPlatformInterface classes, as it may get stomped. Instead, pass
        //! any per-job configuration on the call stack.
            
        const uint32_t m_apiUniqueIndex;
    };
}
