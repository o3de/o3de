/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DevicePipelineState.h>

namespace AZ::WebGPU
{
    class Pipeline;
    class PipelineLayout;

    class PipelineState final
        : public RHI::DevicePipelineState
    {
    public:
        AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);

        static RHI::Ptr<PipelineState> Create();
        PipelineLayout* GetPipelineLayout() const;
        Pipeline* GetPipeline() const;
            
    private:
        PipelineState() = default;
            
        //////////////////////////////////////////////////////////////////////////
        // RHI::DevicePipelineState
        RHI::ResultCode InitInternal(
            RHI::Device& device,
            const RHI::PipelineStateDescriptorForDraw& descriptor,
            RHI::DevicePipelineLibrary* pipelineLibrary) override;
        RHI::ResultCode InitInternal(
            RHI::Device& device,
            const RHI::PipelineStateDescriptorForDispatch& descriptor,
            RHI::DevicePipelineLibrary* pipelineLibrary) override;
        RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::PipelineStateDescriptorForRayTracing& descriptor, [[maybe_unused]] RHI::DevicePipelineLibrary* pipelineLibrary) override { return RHI::ResultCode::Success;}
        void ShutdownInternal() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        RHI::Ptr<Pipeline> m_pipeline;
    };
}
