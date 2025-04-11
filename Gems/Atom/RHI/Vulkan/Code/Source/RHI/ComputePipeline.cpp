/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/ComputePipeline.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<ComputePipeline> AZ::Vulkan::ComputePipeline::Create()
        {
            return aznew ComputePipeline();
        }

        void AZ::Vulkan::ComputePipeline::Shutdown()
        {
            Base::Shutdown();
        }

        RHI::ResultCode AZ::Vulkan::ComputePipeline::InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
        {
            AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline State Dispatch Descriptor is null.");
            AZ_Assert(descriptor.m_pipelineDescritor->GetType() == RHI::PipelineStateType::Dispatch, "Invalid pipeline descriptor type");

            RHI::ResultCode result = BuildNativePipeline(descriptor, pipelineLayout);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            return result;
        }

        VkPipelineShaderStageCreateInfo ComputePipeline::BuildPipelineShaderStageCreateInfo(const RHI::PipelineStateDescriptorForDispatch& descriptor)
        {
            VkPipelineShaderStageCreateInfo createInfo{};
            const ShaderStageFunction* func = static_cast<const ShaderStageFunction*>(descriptor.m_computeFunction.get());
            AZ_Assert(func, "Compute function is null.");
            FillPipelineShaderStageCreateInfo(*func, RHI::ShaderStage::Compute, ShaderSubStage::Default, createInfo);
            return createInfo;
        }

        RHI::ResultCode ComputePipeline::BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
        {
            const auto* computeDescriptor = static_cast<const RHI::PipelineStateDescriptorForDispatch*>(descriptor.m_pipelineDescritor);

            VkComputePipelineCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            createInfo.stage = BuildPipelineShaderStageCreateInfo(*computeDescriptor);
            createInfo.layout = pipelineLayout.GetNativePipelineLayout();
            createInfo.basePipelineHandle = VK_NULL_HANDLE;
            createInfo.basePipelineIndex = -1;

            VkResult result = descriptor.m_device->GetContext().CreateComputePipelines(
                descriptor.m_device->GetNativeDevice(), VK_NULL_HANDLE, 1, &createInfo, VkSystemAllocator::Get(), &GetNativePipelineRef());

            return ConvertResult(result);
        }
    }
}
