/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/Pipeline.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode Pipeline::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline descriptor is null.");
            AZ_Assert(descriptor.m_pipelineDescritor->m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");

            PipelineLayout::Descriptor layoutDescriptor;
            layoutDescriptor.m_device = descriptor.m_device;
            layoutDescriptor.m_pipelineLayoutDescriptor = descriptor.m_pipelineDescritor->m_pipelineLayoutDescriptor;
            RHI::Ptr<PipelineLayout> layout = descriptor.m_device->AcquirePipelineLayout(layoutDescriptor);

            if (!layout)
            {
                AZ_Assert(false, "Failed to acquire PipelineLayout");
                return RHI::ResultCode::Fail;
            }

            Base::Init(*descriptor.m_device);

            RHI::ResultCode result = InitInternal(descriptor, *layout);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_pipelineLayout = layout;
            m_pipelineLibrary = descriptor.m_pipelineLibrary;

            SetName(GetName());
            return result;
        }

        PipelineLayout* Pipeline::GetPipelineLayout() const
        {
            return m_pipelineLayout.get();
        }

        PipelineLibrary* Pipeline::GetPipelineLibrary() const
        {
            return m_pipelineLibrary;
        }

        VkPipeline Pipeline::GetNativePipeline() const
        {
            AZ_Assert(m_nativePipeline != VK_NULL_HANDLE, "Vulkan's native pipeline is null.");
            return m_nativePipeline;
        }

        void Pipeline::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativePipeline), name.data(), VK_OBJECT_TYPE_PIPELINE, static_cast<Device&>(GetDevice()));
            }
        }

        VkPipeline& Pipeline::GetNativePipelineRef()
        {
            return m_nativePipeline;
        }

        void Pipeline::Shutdown()
        {
            m_shaderModules.clear();
            if (m_nativePipeline != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                vkDestroyPipeline(device.GetNativeDevice(), m_nativePipeline, nullptr);
                m_nativePipeline = VK_NULL_HANDLE;
            }
            Base::Shutdown();
        }

        void Pipeline::FillPipelineShaderStageCreateInfo(
            const ShaderStageFunction& function, 
            RHI::ShaderStage stage, 
            uint32_t subStageIndex, 
            VkPipelineShaderStageCreateInfo& createInfo)
        {
            VkShaderStageFlagBits stageBits = VK_SHADER_STAGE_ALL;
            switch (stage)
            {
            case RHI::ShaderStage::Vertex:
                stageBits = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case RHI::ShaderStage::Tessellation:
                switch (subStageIndex)
                {
                case ShaderSubStage::TesselationControl:
                    stageBits = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                    break;
                case ShaderSubStage::TesselationEvaluattion:
                    stageBits = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                    break;
                default:
                    AZ_Assert(false, "Shader Sub Stage is wrong.");
                }
                break;
            case RHI::ShaderStage::Fragment:
                stageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case RHI::ShaderStage::Compute:
                stageBits = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                AZ_Assert(false, "Shader Stage is not correct.");
            }

            const ShaderByteCodeView byteCode = function.GetByteCode(subStageIndex);
            ShaderModule::Descriptor shaderModuleDesc;
            shaderModuleDesc.m_bytecode = byteCode;
            shaderModuleDesc.m_entryFunctionName = function.GetEntryFunctionName(subStageIndex);
            shaderModuleDesc.m_device = static_cast<Device*>(&GetDevice());
            shaderModuleDesc.m_shaderStage = stage;
            shaderModuleDesc.m_shaderSubStage = subStageIndex;
            RHI::Ptr<ShaderModule> shaderModule = ShaderModule::Create();
            shaderModule->Init(shaderModuleDesc);
            m_shaderModules.emplace_back(shaderModule);

            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.stage = stageBits;
            createInfo.module = shaderModule->GetNativeShaderModule();
            createInfo.pName = shaderModule->GetEntryFunctionName().c_str();
            createInfo.pSpecializationInfo = nullptr;
        }
    }
}
