/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDevicePipelineState.h>
#include <RHI/Pipeline.h>
#include <RHI/PipelineLayout.h>

namespace AZ
{
    namespace Vulkan
    {
        class PipelineState final
            : public RHI::SingleDevicePipelineState 
        {
            using Base = RHI::SingleDevicePipelineState;

        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);
            AZ_RTTI(PipelineState, "40D76567-1C43-4D0F-885A-B63337DCE31B", Base);

            static RHI::Ptr<PipelineState> Create();

            PipelineLayout* GetPipelineLayout() const;
            Pipeline* GetPipeline() const;
            PipelineLibrary* GetPipelineLibrary() const;

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDevicePipelineState
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::Ptr<Pipeline> m_pipeline;
        };
    }
}
