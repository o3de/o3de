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
#include <Atom/RHI/DeviceRayTracingBufferPools.h>

namespace AZ::RHI
{
    class BufferPool;

    //! RayTracingBufferPools
    //!
    //! This class encapsulates all of the MultiDeviceBufferPools needed for ray tracing, freeing the application
    //! from setting up and managing the buffers pools individually.
    //!
    class RayTracingBufferPools : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingBufferPools, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingBufferPools, "{59397661-F5A5-44DE-9B1D-E6F5BC32DC51}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingBufferPools);
        RayTracingBufferPools() = default;
        virtual ~RayTracingBufferPools() = default;

        //! Accessors
        const RHI::Ptr<RHI::BufferPool>& GetShaderTableBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetScratchBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetBlasBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetTlasInstancesBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetTlasBufferPool() const;

        //! Initializes the multi-device BufferPools as well as all device-specific DeviceRayTracingBufferPools
        void Init(MultiDevice::DeviceMask deviceMask);

    private:
        bool m_initialized = false;
        RHI::Ptr<RHI::BufferPool> m_ShaderTableBufferPool;
        RHI::Ptr<RHI::BufferPool> m_ScratchBufferPool;
        RHI::Ptr<RHI::BufferPool> m_BlasBufferPool;
        RHI::Ptr<RHI::BufferPool> m_TlasInstancesBufferPool;
        RHI::Ptr<RHI::BufferPool> m_TlasBufferPool;
    };
} // namespace AZ::RHI
