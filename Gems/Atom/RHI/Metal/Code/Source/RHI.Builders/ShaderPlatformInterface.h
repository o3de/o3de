/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RHI.Reflect/Metal/Base.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Metal
    {
        class ShaderPlatformInterface : 
            public RHI::ShaderPlatformInterface
        {
        public:
            explicit ShaderPlatformInterface(uint32_t apiUniqueIndex);

            RHI::APIType GetAPIType() const override;
            AZ::Name GetAPIName() const override;

            RHI::Ptr<RHI::ShaderStageFunction> CreateShaderStageFunction(
                const StageDescriptor& stageDescriptor) override;

            bool IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const override;
            bool IsShaderStageForCompute(RHI::ShaderHardwareStage shaderStageType) const override;
            bool IsShaderStageForRayTracing(RHI::ShaderHardwareStage shaderStageType) const override;

            RHI::Ptr<RHI::PipelineLayoutDescriptor> CreatePipelineLayoutDescriptor() override;

            bool BuildPipelineLayoutDescriptor(
                RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor,
                const ShaderResourceGroupInfoList& srgInfoList,
                const RootConstantsInfo& rootConstantsInfo,
                const RHI::ShaderBuildArguments& shaderBuildArguments) override;
            
            bool VariantCompilationRequiresSrgLayoutData() const override { return true; }

            bool CompilePlatformInternal(
                const AssetBuilderSDK::PlatformInfo& platform,
                const AZStd::string& shaderSourcePath,
                const AZStd::string& functionName,
                RHI::ShaderHardwareStage shaderStage,
                const AZStd::string& tempFolderPath,
                StageDescriptor& outputDescriptor,
                const RHI::ShaderBuildArguments& shaderBuildArguments,
                const bool useSpecializationConstants) const override; 

            const char* GetAzslHeader(const AssetBuilderSDK::PlatformInfo& platform) const override;

        private:
            ShaderPlatformInterface() = delete;

            bool CompileHLSLShader(
                const AZStd::string& shaderSourceFile,
                const AZStd::string& tempFolder,
                const AZStd::string& entryPoint,
                const RHI::ShaderHardwareStage shaderAssetType,
                const RHI::ShaderBuildArguments& shaderBuildArguments,
                AZStd::vector<char>& compiledShader,
                AZStd::vector<uint8_t>& compiledByteCode,
                const AssetBuilderSDK::PlatformInfo& platform,
                const bool isGraphicsDevModeEnabled,
                ByProducts& byproducts) const;

            bool UpdateCompiledShader(AZ::IO::FileIOStream& fileStream,
                                      const char* platformName,
                                      const char* fileName,
                                      AZStd::vector<char>& compiledShader) const;
            
            bool CreateMetalLib(const char* platformName,
                                const AZStd::string& shaderSourceFile,
                                const AZStd::string& tempFolder,
                                AZStd::vector<uint8_t>& compiledByteCode,
                                AZStd::vector<char>& sourceMetalShader,
                                const AssetBuilderSDK::PlatformInfo& platform,
                                const RHI::ShaderBuildArguments& shaderBuildArguments,
                                const bool isGraphicsDevModeEnabled,
                                RHI::ShaderHardwareStage shaderStageType) const;
            
            // Updates the metal shader for using subpass inputs
            void UpdateMetalSource(AZStd::string& metalSource, RHI::ShaderHardwareStage shaderStageType) const;
            
            using ArgBufferEntries = AZStd::pair<AZStd::string, uint32_t>;
            struct compareByRegisterId {
                bool operator()(const ArgBufferEntries& lhs, const ArgBufferEntries& rhs)
                {
                    return lhs.second < rhs.second;
                }
            };
            
            /**
             * These set of functions will parse the final metal source code and add unused variables
             * within the declaration of argument buffers so that they match ShaderResourceGroupLayout
             * This is necessary because metal drivers expect the argbuffer layout bound at runtime to match
             * the one declared in the shaders or we get undefined behaviour (hard crashes/weird lags/etc).
             *
             * For example - Given SRG like so
             * ShaderResourceGroup ObjectSrg : SRG_PerObject
             * {
             *     Texture2D<float4> FontImage;
             *
             *     Sampler LinearSampler
             *     {
             *         MinFilter = Linear;
             *         MagFilter = Linear;
             *         MipFilter = Linear;
             *         AddressU = Clamp;
             *         AddressV = Clamp;
             *         AddressW = Clamp;
             *     };
             *
             *     column_major float4x4 m_projectionMatrix;
             * }
             *
             * the imgui vertex shader that only uses m_projectionMatrix will modify the following metal shader
             *
             * struct spvDescriptorSetBuffer0
             * {
             *    constant type_ConstantBuffer_PerObject_SRGConstantsStruct* PerObject_SRGConstantBuffer [[id(2)]];
             * };
             *
             * to
             *
             * struct spvDescriptorSetBuffer0
             * {
             *     texture2d<float> dummyImage0 [[id(0)]];
             *     sampler dummySampler1 [[id(1)]];
             *     constant type_ConstantBuffer_PerObject_SRGConstantsStruct* PerObject_SRGConstantBuffer [[id(2)]];
             * };
             */
            bool AddUnusedResources(AZStd::vector<char>& compiledShader) const;
            
            //Insert all the static sampler resource string entries into a set.
            bool AddSamplerEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                         AZStd::string& argBufferStr) const;
            
            //! Insert all the image resource string entries into the set. Use ProcessSamplerEntry as a helper method.
            bool AddImageEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                AZStd::string& argBufferStr) const;
            bool ProcessSamplerEntry(uint32_t regId, AZStd::string& argBufferStr, uint32_t samplercount) const;
            
            //! Insert all the constant buffer resource string entries into the set.
            bool AddConstantBufferEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                          AZStd::string& constantBufferTempStructs,
                                          AZStd::string& argBufferStr,
                                          uint32_t groupLayoutIndex) const;
            
            //! Insert all the buffer resource string entries into the set.
            bool AddBufferEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                            AZStd::string& structuredBufferTempStructs,
                                            AZStd::string& argBufferStr,
                                            uint32_t groupLayoutIndex) const;
            
            //! Helper function to add already existing resource entry into the set
            bool AddExistingResourceEntry(const char* resourceStr,
                                          size_t resourceStartPos,
                                          uint32_t regId,
                                          AZStd::string& argBufferStr) const;
            
            //! This is to cache the srg layout which is needed to add the unused variables
            mutable AZStd::fixed_vector<const RHI::ShaderResourceGroupLayout*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgLayouts;
            
            //! The set helps remove duplicates and keeps the entries sorted based on register id
            mutable AZStd::set<ArgBufferEntries, compareByRegisterId> m_argBufferEntries;

            const Name m_apiName{APINameString};
        };
    }
}
