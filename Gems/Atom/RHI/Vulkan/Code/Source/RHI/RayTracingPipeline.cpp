/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/RayTracingPipeline.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingPipeline> AZ::Vulkan::RayTracingPipeline::Create()
        {
            return aznew RayTracingPipeline();
        }

        void AZ::Vulkan::RayTracingPipeline::Shutdown()
        {
            Base::Shutdown();
        }

        RHI::ResultCode AZ::Vulkan::RayTracingPipeline::InitInternal([[maybe_unused]] const Descriptor& descriptor, const PipelineLayout&)
        {
            AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline State Dispatch Descriptor is null.");
            AZ_Assert(descriptor.m_pipelineDescritor->GetType() == RHI::PipelineStateType::RayTracing, "Invalid pipeline descriptor type");

            return RHI::ResultCode::Success;
        }

        VkPipelineShaderStageCreateInfo RayTracingPipeline::BuildPipelineShaderStageCreateInfo(const RHI::PipelineStateDescriptorForRayTracing& descriptor)
        {
            VkPipelineShaderStageCreateInfo createInfo{};
            const ShaderStageFunction* func = static_cast<const ShaderStageFunction*>(descriptor.m_rayTracingFunction.get());
            AZ_Assert(func, "RayTracing function is null.");
            FillPipelineShaderStageCreateInfo(*func, RHI::ShaderStage::RayTracing, ShaderSubStage::Default, createInfo);
            return createInfo;
        }
    }
}
