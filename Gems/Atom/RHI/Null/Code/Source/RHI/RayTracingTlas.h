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
#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingTlas final
            : public RHI::SingleDeviceRayTracingTlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingTlas> Create();
            
            // RHI::SingleDeviceRayTracingTlas overrides...
            const RHI::Ptr<RHI::SingleDeviceBuffer> GetTlasBuffer() const override { return nullptr; }
            const RHI::Ptr<RHI::SingleDeviceBuffer> GetTlasInstancesBuffer() const override { return nullptr; }

        private:
            RayTracingTlas() = default;

            // RHI::SingleDeviceRayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::SingleDeviceRayTracingTlasDescriptor* descriptor, [[maybe_unused]] const RHI::SingleDeviceRayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}

        };
    }
}
