/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/RayTracingPipeline.h>
#include <RHI/Device.h>
#include <RHI/Conversion.h>
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
