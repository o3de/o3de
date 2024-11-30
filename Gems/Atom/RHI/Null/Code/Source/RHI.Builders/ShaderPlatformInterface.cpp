/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RHI.Builders/ShaderPlatformInterface.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Reflect/Null/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Null/ShaderStageFunction.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Null
    {

        static const char* WindowsAzslShaderHeader = "Builders/ShaderHeaders/Platform/Windows/Null/AzslcHeader.azsli";
        static const char* MacAzslShaderHeader = "Builders/ShaderHeaders/Platform/Mac/Null/AzslcHeader.azsli";

        ShaderPlatformInterface::ShaderPlatformInterface(uint32_t apiUniqueIndex)
            : RHI::ShaderPlatformInterface(apiUniqueIndex)
        {
        }

        RHI::APIType ShaderPlatformInterface::GetAPIType() const
        {
            return Null::RHIType;
        }

        AZ::Name ShaderPlatformInterface::GetAPIName() const
        {
            return m_apiName;
        }

        RHI::Ptr <RHI::PipelineLayoutDescriptor> ShaderPlatformInterface::CreatePipelineLayoutDescriptor()
        {
            return AZ::Null::PipelineLayoutDescriptor::Create();
        }
 
        bool ShaderPlatformInterface::BuildPipelineLayoutDescriptor(
            [[maybe_unused]] RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptorBase,
            [[maybe_unused]] const ShaderResourceGroupInfoList& srgInfoList,
            [[maybe_unused]] const RootConstantsInfo& rootConstantsInfo,
            [[maybe_unused]] const RHI::ShaderBuildArguments& shaderBuildArguments)
        {
            PipelineLayoutDescriptor* pipelineLayoutDescriptor = azrtti_cast<PipelineLayoutDescriptor*>(pipelineLayoutDescriptorBase.get());
            AZ_Assert(pipelineLayoutDescriptor, "PipelineLayoutDescriptor should have been created by now");
            return pipelineLayoutDescriptor->Finalize() == RHI::ResultCode::Success;
        }
    
        RHI::Ptr<RHI::ShaderStageFunction> ShaderPlatformInterface::CreateShaderStageFunction(const StageDescriptor& stageDescriptor)
        {
            RHI::Ptr<ShaderStageFunction> newShaderStageFunction = ShaderStageFunction::Create(RHI::ToRHIShaderStage(stageDescriptor.m_stageType));
            newShaderStageFunction->Finalize();
            return newShaderStageFunction;
        }

        bool ShaderPlatformInterface::IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const
        {
            bool hasRasterProgram = false;

            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Vertex;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Fragment;

            return hasRasterProgram;
        }

        bool ShaderPlatformInterface::IsShaderStageForCompute(RHI::ShaderHardwareStage shaderStageType) const
        {
            return (shaderStageType == RHI::ShaderHardwareStage::Compute);
        }

        bool ShaderPlatformInterface::IsShaderStageForRayTracing(RHI::ShaderHardwareStage shaderStageType) const
        {
            return (shaderStageType == RHI::ShaderHardwareStage::RayTracing);
        }

        const char* ShaderPlatformInterface::GetAzslHeader([[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform) const
        {
            if (platform.m_identifier == "pc")
            {
                return WindowsAzslShaderHeader;
            }
            else if (platform.m_identifier == "mac")
            {
                return MacAzslShaderHeader;
            }
            else
            {
                return WindowsAzslShaderHeader;
            }
        }

        bool ShaderPlatformInterface::CompilePlatformInternal(
            [[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform, [[maybe_unused]] const AZStd::string& shaderSourcePath,
            [[maybe_unused]] const AZStd::string& functionName, [[maybe_unused]] RHI::ShaderHardwareStage shaderStage,
            [[maybe_unused]] const AZStd::string& tempFolderPath, [[maybe_unused]] StageDescriptor& outputDescriptor,
            [[maybe_unused]] const RHI::ShaderBuildArguments& shaderBuildArguments,
            [[maybe_unused]] const bool useSpecializationConstants) const
        {
            outputDescriptor.m_stageType = shaderStage;
            return true;
        }
    }
}
