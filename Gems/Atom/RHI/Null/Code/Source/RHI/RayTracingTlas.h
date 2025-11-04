/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingTlas final
            : public RHI::DeviceRayTracingTlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingTlas> Create();
            
            // RHI::DeviceRayTracingTlas overrides...
            const RHI::Ptr<RHI::DeviceBuffer> GetTlasBuffer() const override { return nullptr; }
            const RHI::Ptr<RHI::DeviceBuffer> GetTlasInstancesBuffer() const override { return nullptr; }

        private:
            RayTracingTlas() = default;

            // RHI::DeviceRayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::DeviceRayTracingTlasDescriptor* descriptor, [[maybe_unused]] const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}

        };
    }
}
