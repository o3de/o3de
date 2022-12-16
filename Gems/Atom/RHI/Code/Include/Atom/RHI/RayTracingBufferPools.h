/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/BufferPool.h>

namespace AZ
{
    namespace RHI
    {
        class Device;

        //! RayTracingBufferPools
        //!
        //! This class encapsulates all of the BufferPools needed for ray tracing, freeing the application
        //! from setting up and managing the buffers pools individually.
        //!
        class RayTracingBufferPools : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBufferPools, AZ::SystemAllocator, 0);
            AZ_RTTI(RayTracingBufferPools, "{091B2839-3148-4284-882B-B82B280299E4}", MultiDeviceObject)
            RayTracingBufferPools() = default;
            virtual ~RayTracingBufferPools() = default;

            static RHI::Ptr<RayTracingBufferPools> Create()
            {
                return aznew RayTracingBufferPools;
            }

            // accessors
            const RHI::Ptr<RHI::BufferPool>& GetShaderTableBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetScratchBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetBlasBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetTlasInstancesBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetTlasBufferPool() const;

            // operations
            void Init(DeviceMask deviceMask);

            inline const RHI::Ptr<DeviceRayTracingBufferPools>& GetDeviceRayTracingBufferPool(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceRayTracingBufferPools.find(deviceIndex) != m_deviceRayTracingBufferPools.end(),
                    "No DeviceRayTracingBufferPools found for device index %d\n",
                    deviceIndex);
                return m_deviceRayTracingBufferPools.at(deviceIndex);
            }

        private:
            bool m_initialized = false;
            RHI::Ptr<RHI::BufferPool> m_shaderTableBufferPool;
            RHI::Ptr<RHI::BufferPool> m_scratchBufferPool;
            RHI::Ptr<RHI::BufferPool> m_blasBufferPool;
            RHI::Ptr<RHI::BufferPool> m_tlasInstancesBufferPool;
            RHI::Ptr<RHI::BufferPool> m_tlasBufferPool;

            AZStd::unordered_map<int, Ptr<DeviceRayTracingBufferPools>> m_deviceRayTracingBufferPools;
        };
    } // namespace RHI
} // namespace AZ
