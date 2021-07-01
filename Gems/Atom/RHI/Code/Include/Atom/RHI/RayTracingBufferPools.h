/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        class BufferPool;        
        class Device;

        //! RayTracingBufferPools
        //!
        //! This class encapsulates all of the BufferPools needed for ray tracing, freeing the application
        //! from setting up and managing the buffers pools individually.
        //!
        class RayTracingBufferPools
            : public DeviceObject
        {
        public:
            virtual ~RayTracingBufferPools() = default;

            static RHI::Ptr<RHI::RayTracingBufferPools> CreateRHIRayTracingBufferPools();

            // accessors
            const RHI::Ptr<RHI::BufferPool>& GetShaderTableBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetScratchBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetBlasBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetTlasInstancesBufferPool() const;
            const RHI::Ptr<RHI::BufferPool>& GetTlasBufferPool() const;

            // operations
            void Init(RHI::Ptr<RHI::Device>& device);

        protected:
            RayTracingBufferPools() = default;

            virtual RHI::BufferBindFlags GetShaderTableBufferBindFlags() const { return RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable; }
            virtual RHI::BufferBindFlags GetScratchBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite; }
            virtual RHI::BufferBindFlags GetBlasBufferBindFlags() const { return RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure; }
            virtual RHI::BufferBindFlags GetTlasInstancesBufferBindFlags() const { return RHI::BufferBindFlags::ShaderRead; }
            virtual RHI::BufferBindFlags GetTlasBufferBindFlags() const { return RHI::BufferBindFlags::RayTracingAccelerationStructure; }

        private:
            bool m_initialized = false;
            RHI::Ptr<RHI::BufferPool> m_shaderTableBufferPool;
            RHI::Ptr<RHI::BufferPool> m_scratchBufferPool;
            RHI::Ptr<RHI::BufferPool> m_blasBufferPool;
            RHI::Ptr<RHI::BufferPool> m_tlasInstancesBufferPool;
            RHI::Ptr<RHI::BufferPool> m_tlasBufferPool;
        };
    }
}
