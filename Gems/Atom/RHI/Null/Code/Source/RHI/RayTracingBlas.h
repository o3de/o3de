/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/SingleDeviceRayTracingBufferPools.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingBlas final
            : public RHI::SingleDeviceRayTracingBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingBlas> Create();

            // RHI::SingleDeviceRayTracingBlas overrides...
            virtual bool IsValid() const override { return true; }

        private:
            RayTracingBlas() = default;

            // RHI::SingleDeviceRayTracingBlas overrides...
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::SingleDeviceRayTracingBlasDescriptor* descriptor, [[maybe_unused]] const RHI::SingleDeviceRayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}
        };
    }
}
