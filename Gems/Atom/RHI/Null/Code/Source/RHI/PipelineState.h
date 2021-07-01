/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
