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

namespace AZ
{
    namespace Null
    {
        class RayTracingClusterBlas final : public RHI::DeviceRayTracingClusterBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingClusterBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingClusterBlas> Create()
            {
                return aznew RayTracingClusterBlas;
            }

            uint64_t GetAccelerationStructureByteSize() override
            {
                return 0;
            }

        private:
            RayTracingClusterBlas() = default;

            // RHI::DeviceRayTracingClusterBlas overrides
            RHI::ResultCode CreateBuffersInternal(
                [[maybe_unused]] RHI::Device& deviceBase,
                [[maybe_unused]] const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor,
                [[maybe_unused]] const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override
            {
                return RHI::ResultCode::Success;
            }
        };
    } // namespace Null
} // namespace AZ
