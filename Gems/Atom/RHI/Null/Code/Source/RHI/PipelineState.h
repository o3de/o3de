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

#include <Atom/RHI/PipelineState.h>

namespace AZ
{
    namespace Null
    {
        class PipelineState final
            : public RHI::PipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator, 0);

            static RHI::Ptr<PipelineState> Create();
            
        private:
            PipelineState() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineState
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::PipelineStateDescriptorForDraw& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::PipelineStateDescriptorForDispatch& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::PipelineStateDescriptorForRayTracing& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
