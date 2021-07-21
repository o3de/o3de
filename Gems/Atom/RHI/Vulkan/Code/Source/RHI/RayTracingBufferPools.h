/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace Vulkan
    {
        //! This is the Vulkan-specific RayTracingBufferPools class.
        class RayTracingBufferPools final
            : public RHI::RayTracingBufferPools
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBufferPools, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingBufferPools> Create() { return aznew RayTracingBufferPools; }

        protected:
            virtual RHI::BufferBindFlags GetShaderTableBufferBindFlags() const override { return RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable; }
            virtual RHI::BufferBindFlags GetTlasInstancesBufferBindFlags() const override { return RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::RayTracingAccelerationStructure; }

        private:
            RayTracingBufferPools() = default;
        };
    }
}
