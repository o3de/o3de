/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceBufferPool.h>

namespace AZ::RHI
{
    class DeviceBufferPool;
    class Device;

    //! DeviceRayTracingBufferPools
    //!
    //! This class encapsulates all of the BufferPools needed for ray tracing, freeing the application
    //! from setting up and managing the buffers pools individually.
    //!
    class DeviceRayTracingBufferPools
        : public DeviceObject
    {
    public:
        virtual ~DeviceRayTracingBufferPools() = default;

        static RHI::Ptr<RHI::DeviceRayTracingBufferPools> CreateRHIRayTracingBufferPools();

        //! Accessors
        const RHI::Ptr<RHI::DeviceBufferPool>& GetShaderTableBufferPool() const;
        const RHI::Ptr<RHI::DeviceBufferPool>& GetScratchBufferPool() const;
        const RHI::Ptr<RHI::DeviceBufferPool>& GetAabbStagingBufferPool() const;
        const RHI::Ptr<RHI::DeviceBufferPool>& GetBlasBufferPool() const;
        const RHI::Ptr<RHI::DeviceBufferPool>& GetTlasInstancesBufferPool() const;
        const RHI::Ptr<RHI::DeviceBufferPool>& GetTlasBufferPool() const;

        // operations
        void Init(RHI::Ptr<RHI::Device>& device);

    protected:
        DeviceRayTracingBufferPools() = default;

        virtual RHI::BufferBindFlags GetShaderTableBufferBindFlags() const { return RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable; }
        virtual RHI::BufferBindFlags GetScratchBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingScratchBuffer; }
        virtual RHI::BufferBindFlags GetAabbStagingBufferBindFlags() const { return RHI::BufferBindFlags::CopyRead; }
        virtual RHI::BufferBindFlags GetBlasBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure; }
        virtual RHI::BufferBindFlags GetTlasInstancesBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite; }
        virtual RHI::BufferBindFlags GetTlasBufferBindFlags() const { return RHI::BufferBindFlags::RayTracingAccelerationStructure; }

    private:
        bool m_initialized = false;
        RHI::Ptr<RHI::DeviceBufferPool> m_shaderTableBufferPool;
        RHI::Ptr<RHI::DeviceBufferPool> m_scratchBufferPool;
        RHI::Ptr<RHI::DeviceBufferPool> m_aabbStagingBufferPool;
        RHI::Ptr<RHI::DeviceBufferPool> m_blasBufferPool;
        RHI::Ptr<RHI::DeviceBufferPool> m_tlasInstancesBufferPool;
        RHI::Ptr<RHI::DeviceBufferPool> m_tlasBufferPool;
    };
}
