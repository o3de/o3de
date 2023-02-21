/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(RayTracingPipeline, AZ::ThreadPoolAllocator);
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
