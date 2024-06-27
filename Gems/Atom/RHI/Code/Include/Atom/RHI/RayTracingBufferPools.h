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
    //! This class encapsulates all of the BufferPools needed for ray tracing, freeing the application
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
        const RHI::Ptr<RHI::BufferPool>& GetAabbStagingBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetBlasBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetTlasInstancesBufferPool() const;
        const RHI::Ptr<RHI::BufferPool>& GetTlasBufferPool() const;

        //! Initializes the multi-device BufferPools as well as all device-specific DeviceRayTracingBufferPools
        void Init(MultiDevice::DeviceMask deviceMask);

    protected:
        virtual RHI::BufferBindFlags GetShaderTableBufferBindFlags() const { return RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable; }
        virtual RHI::BufferBindFlags GetScratchBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingScratchBuffer; }
        virtual RHI::BufferBindFlags GetAabbStagingBufferBindFlags() const { return RHI::BufferBindFlags::CopyRead; }
        virtual RHI::BufferBindFlags GetBlasBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure; }
        virtual RHI::BufferBindFlags GetTlasInstancesBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite; }
        virtual RHI::BufferBindFlags GetTlasBufferBindFlags() const { return RHI::BufferBindFlags::RayTracingAccelerationStructure; }

    private:
        bool m_initialized = false;
        RHI::Ptr<RHI::BufferPool> m_shaderTableBufferPool;
        RHI::Ptr<RHI::BufferPool> m_scratchBufferPool;
        RHI::Ptr<RHI::BufferPool> m_aabbStagingBufferPool;
        RHI::Ptr<RHI::BufferPool> m_blasBufferPool;
        RHI::Ptr<RHI::BufferPool> m_tlasInstancesBufferPool;
        RHI::Ptr<RHI::BufferPool> m_tlasBufferPool;
    };
} // namespace AZ::RHI
