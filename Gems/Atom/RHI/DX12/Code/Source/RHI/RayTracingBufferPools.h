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
    namespace DX12
    {
        //! This is the DX12-specific RayTracingBufferPools class.
        class RayTracingBufferPools final
            : public RHI::DeviceRayTracingBufferPools
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBufferPools, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingBufferPools> Create() { return aznew RayTracingBufferPools; }

        private:
            RayTracingBufferPools() = default;
        };
    }
}
