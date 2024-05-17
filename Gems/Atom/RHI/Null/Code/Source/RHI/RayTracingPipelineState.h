/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceRayTracingPipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Null
    {
        class RayTracingPipelineState final
            : public RHI::SingleDeviceRayTracingPipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingPipelineState, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingPipelineState> Create();

        private:
            RayTracingPipelineState() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDevicePipelineState
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::SingleDeviceRayTracingPipelineStateDescriptor* descriptor) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
