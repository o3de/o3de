/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/SingleDeviceBufferPool.h>

namespace AZ::RHI
{
    class SingleDeviceBufferPool;        
    class Device;

    //! SingleDeviceRayTracingBufferPools
    //!
    //! This class encapsulates all of the BufferPools needed for ray tracing, freeing the application
    //! from setting up and managing the buffers pools individually.
    //!
    class SingleDeviceRayTracingBufferPools
        : public DeviceObject
    {
    public:
        virtual ~SingleDeviceRayTracingBufferPools() = default;

        static RHI::Ptr<RHI::SingleDeviceRayTracingBufferPools> CreateRHIRayTracingBufferPools();

        // accessors
        const RHI::Ptr<RHI::SingleDeviceBufferPool>& GetShaderTableBufferPool() const;
        const RHI::Ptr<RHI::SingleDeviceBufferPool>& GetScratchBufferPool() const;
        const RHI::Ptr<RHI::SingleDeviceBufferPool>& GetBlasBufferPool() const;
        const RHI::Ptr<RHI::SingleDeviceBufferPool>& GetTlasInstancesBufferPool() const;
        const RHI::Ptr<RHI::SingleDeviceBufferPool>& GetTlasBufferPool() const;

        // operations
        void Init(RHI::Ptr<RHI::Device>& device);

    protected:
        SingleDeviceRayTracingBufferPools() = default;

        virtual RHI::BufferBindFlags GetShaderTableBufferBindFlags() const { return RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable; }
        virtual RHI::BufferBindFlags GetScratchBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingScratchBuffer; }
        virtual RHI::BufferBindFlags GetBlasBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure; }
        virtual RHI::BufferBindFlags GetTlasInstancesBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite; }
        virtual RHI::BufferBindFlags GetTlasBufferBindFlags() const { return RHI::BufferBindFlags::RayTracingAccelerationStructure; }

    private:
        bool m_initialized = false;
        RHI::Ptr<RHI::SingleDeviceBufferPool> m_shaderTableBufferPool;
        RHI::Ptr<RHI::SingleDeviceBufferPool> m_scratchBufferPool;
        RHI::Ptr<RHI::SingleDeviceBufferPool> m_blasBufferPool;
        RHI::Ptr<RHI::SingleDeviceBufferPool> m_tlasInstancesBufferPool;
        RHI::Ptr<RHI::SingleDeviceBufferPool> m_tlasBufferPool;
    };
}
