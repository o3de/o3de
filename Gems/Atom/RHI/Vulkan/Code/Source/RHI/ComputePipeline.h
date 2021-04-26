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
        class ComputePipeline final
            : public Pipeline
        {
            using Base = Pipeline;

        public:
            AZ_CLASS_ALLOCATOR(ComputePipeline, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(ComputePipeline, "1D7640F2-3798-4174-AE81-0232C9F745FC", Base);

            static RHI::Ptr<ComputePipeline> Create();
            ~ComputePipeline() {}

        private:
            ComputePipeline() = default;

            //////////////////////////////////////////////////////////////////////////
            //! RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //! Pipeline
            RHI::ResultCode InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout) override;
            RHI::PipelineStateType GetType() override { return RHI::PipelineStateType::Dispatch; }
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout);
            VkPipelineShaderStageCreateInfo BuildPipelineShaderStageCreateInfo(const RHI::PipelineStateDescriptorForDispatch& descriptor);
        };

    }
}
