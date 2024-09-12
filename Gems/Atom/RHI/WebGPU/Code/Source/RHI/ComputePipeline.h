/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Pipeline.h>

namespace AZ::WebGPU
{
    class ComputePipeline final
        : public Pipeline
    {
        using Base = Pipeline;

    public:
        AZ_CLASS_ALLOCATOR(ComputePipeline, AZ::ThreadPoolAllocator);
        AZ_RTTI(ComputePipeline, "{994A01EE-4718-4A64-A928-AA550F28EC46}", Base);

        static RHI::Ptr<ComputePipeline> Create();
        ~ComputePipeline() {}

        const wgpu::ComputePipeline& GetNativeComputePipeline() const;

    private:
        ComputePipeline() = default;

        //////////////////////////////////////////////////////////////////////////
        //! RHI::DeviceObject
        void Shutdown() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! Pipeline
        RHI::ResultCode InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout) override;
        RHI::PipelineStateType GetType() override { return RHI::PipelineStateType::Dispatch; }
        //////////////////////////////////////////////////////////////////////////

        RHI::ResultCode BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout);

        //! Native compute pipeline
        wgpu::ComputePipeline m_wgpuComputePipeline = nullptr;
        AZStd::vector<wgpu::ConstantEntry> m_computeConstants;
    };
}
