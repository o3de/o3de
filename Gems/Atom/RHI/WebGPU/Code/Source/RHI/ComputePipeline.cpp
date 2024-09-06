/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/ComputePipeline.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>

namespace AZ::WebGPU
{
    RHI::Ptr<ComputePipeline> ComputePipeline::Create()
    {
        return aznew ComputePipeline();
    }

    void ComputePipeline::Shutdown()
    {
        Base::Shutdown();
    }

    RHI::ResultCode ComputePipeline::InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
    {
        AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline State Dispatch Descriptor is null.");
        AZ_Assert(descriptor.m_pipelineDescritor->GetType() == RHI::PipelineStateType::Dispatch, "Invalid pipeline descriptor type");

        RHI::ResultCode result = BuildNativePipeline(descriptor, pipelineLayout);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        return result;
    }

    RHI::ResultCode ComputePipeline::BuildNativePipeline(
        [[maybe_unused]] const Descriptor& descriptor, [[maybe_unused]] const PipelineLayout& pipelineLayout)
    {
        return RHI::ResultCode::Success;
    }
}
