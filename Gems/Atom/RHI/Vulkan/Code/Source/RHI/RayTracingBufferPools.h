/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>

namespace AZ
{
    namespace Vulkan
    {
        //! This is the Vulkan-specific RayTracingBufferPools class.
        class RayTracingBufferPools final
            : public RHI::DeviceRayTracingBufferPools
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBufferPools, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingBufferPools> Create() { return aznew RayTracingBufferPools; }

        protected:
            RHI::BufferBindFlags GetAabbStagingBufferBindFlags() const override { return RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingAccelerationStructure; }
            RHI::BufferBindFlags GetShaderTableBufferBindFlags() const override { return RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable; }
            RHI::BufferBindFlags GetTlasInstancesBufferBindFlags() const override { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure; }

        private:
            RayTracingBufferPools() = default;
        };
    }
}
