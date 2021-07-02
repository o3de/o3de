/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingBlas final
            : public RHI::RayTracingBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingBlas> Create();

            // RHI::RayTracingBlas overrides...
            virtual bool IsValid() const override { return true; }

        private:
            RayTracingBlas() = default;

            // RHI::RayTracingBlas overrides...
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingBlasDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}
        };
    }
}
