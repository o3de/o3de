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
#pragma once

#include <RHI/Pipeline.h>

namespace AZ
{
    namespace Vulkan
    {
        class RayTracingPipeline final
            : public Pipeline
        {
            using Base = Pipeline;

        public:
            AZ_CLASS_ALLOCATOR(RayTracingPipeline, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(RayTracingPipeline, "681BF246-D5DC-4155-9300-4B2CE5B8BDC7", Base);

            static RHI::Ptr<RayTracingPipeline> Create();
            ~RayTracingPipeline() {}

        private:
            RayTracingPipeline() = default;

            //////////////////////////////////////////////////////////////////////////
            //! RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //! Pipeline
            RHI::ResultCode InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout) override;
            RHI::PipelineStateType GetType() override { return RHI::PipelineStateType::RayTracing; }
            //////////////////////////////////////////////////////////////////////////

            VkPipelineShaderStageCreateInfo BuildPipelineShaderStageCreateInfo(const RHI::PipelineStateDescriptorForRayTracing& descriptor);
        };

    }
}
