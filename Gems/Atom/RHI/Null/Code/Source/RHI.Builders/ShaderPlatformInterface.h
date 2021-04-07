/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RHI.Reflect/Null/Base.h>

namespace AZ
{
    namespace Null
    {
        class ShaderPlatformInterface : 
            public RHI::ShaderPlatformInterface
        {
        public:
            explicit ShaderPlatformInterface(uint32_t apiUniqueIndex);

            //////////////////////////////////////////////////////////////////////////
            // RHI::ShaderPlatformInterface
            RHI::APIType GetAPIType() const override;
            AZ::Name GetAPIName() const override;
            RHI::Ptr<RHI::PipelineLayoutDescriptor> CreatePipelineLayoutDescriptor() override;
            RHI::Ptr<RHI::ShaderStageFunction> CreateShaderStageFunction(const StageDescriptor& stageDescriptor) override;
            bool IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const override;
            bool IsShaderStageForCompute(RHI::ShaderHardwareStage shaderStageType) const override;
            bool IsShaderStageForRayTracing(RHI::ShaderHardwareStage shaderStageType) const override;
            bool CompilePlatformInternal(
                const AssetBuilderSDK::PlatformInfo& platform, const AZStd::string& shaderSourcePath, const AZStd::string& functionName,
                RHI::ShaderHardwareStage shaderStage, const AZStd::string& tempFolderPath, StageDescriptor& outputDescriptor,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments) const override;
            AZStd::string GetAzslCompilerWarningParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const;
            bool BuildHasDebugInfo(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const override;
            const char* GetAzslHeader(const AssetBuilderSDK::PlatformInfo& platform) const override;
            bool BuildPipelineLayoutDescriptor(
                RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor,
                const ShaderResourceGroupInfoList& srgInfoList,
                const RootConstantsInfo& rootConstantsInfo,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments) override;
            AZStd::string GetAzslCompilerParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const override;
            //////////////////////////////////////////////////////////////////////////
            
        private:
            ShaderPlatformInterface() = delete;

            const Name m_apiName{APINameString};
        };
    }
}
