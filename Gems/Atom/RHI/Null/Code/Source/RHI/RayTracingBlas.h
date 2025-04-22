/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingBlas final
            : public RHI::DeviceRayTracingBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingBlas> Create();

            // RHI::DeviceRayTracingBlas overrides...
            virtual bool IsValid() const override { return true; }

            uint64_t GetAccelerationStructureByteSize() override
            {
                return 0;
            }

        private:
            RayTracingBlas() = default;

            // RHI::DeviceRayTracingBlas overrides...
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::DeviceRayTracingBlasDescriptor* descriptor, [[maybe_unused]] const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}
            RHI::ResultCode CreateCompactedBuffersInternal(
                [[maybe_unused]] RHI::Device& device,
                [[maybe_unused]] RHI::Ptr<RHI::DeviceRayTracingBlas> sourceBlas,
                [[maybe_unused]] uint64_t compactedBufferSize,
                [[maybe_unused]] const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override
            {
                return RHI::ResultCode::Success;
            }
        };
    }
}
