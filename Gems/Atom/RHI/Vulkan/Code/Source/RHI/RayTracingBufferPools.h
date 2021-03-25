/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
