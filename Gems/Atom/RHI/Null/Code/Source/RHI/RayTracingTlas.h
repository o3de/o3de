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
#include <Atom/RHI/RayTracingAccelerationStructure.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingTlas final
            : public RHI::RayTracingTlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingTlas> Create();
            
            // RHI::RayTracingTlas overrides...
            virtual const RHI::Ptr<RHI::Buffer> GetTlasBuffer() const override { return nullptr; }

        private:
            RayTracingTlas() = default;

            // RHI::RayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingTlasDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}

        };
    }
}
