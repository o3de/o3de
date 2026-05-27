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
    namespace DX12
    {
        class Buffer;

        //! This class builds and contains the DX12 RayTracing BLAS buffers.
        class RayTracingClusterBlas final : public RHI::DeviceRayTracingClusterBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingClusterBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingClusterBlas> Create();

            uint64_t GetAccelerationStructureByteSize() override;

        private:
            RayTracingClusterBlas() = default;

            // RHI::DeviceRayTracingBlas overrides
            RHI::ResultCode CreateBuffersInternal(
                RHI::Device& deviceBase,
                const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor,
                const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;
        };
    } // namespace DX12
} // namespace AZ
