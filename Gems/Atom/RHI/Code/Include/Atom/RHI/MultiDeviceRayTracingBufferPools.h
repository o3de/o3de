/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/MultiDeviceBufferPool.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDeviceBufferPool;

        //! MultiDeviceRayTracingBufferPools
        //!
        //! This class encapsulates all of the BufferPools needed for ray tracing, freeing the application
        //! from setting up and managing the buffers pools individually.
        //!
        class MultiDeviceRayTracingBufferPools : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingBufferPools, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceRayTracingBufferPools, "{59397661-F5A5-44DE-9B1D-E6F5BC32DC51}", MultiDeviceObject)
            MultiDeviceRayTracingBufferPools() = default;
            virtual ~MultiDeviceRayTracingBufferPools() = default;

            // accessors
            const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetShaderTableBufferPool() const;
            const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetScratchBufferPool() const;
            const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetBlasBufferPool() const;
            const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetTlasInstancesBufferPool() const;
            const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetTlasBufferPool() const;

            // operations
            void Init(MultiDevice::DeviceMask deviceMask);

            inline const RHI::Ptr<RayTracingBufferPools>& GetDeviceRayTracingBufferPool(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceRayTracingBufferPools",
                    m_deviceRayTracingBufferPools.find(deviceIndex) != m_deviceRayTracingBufferPools.end(),
                    "No RayTracingBufferPools found for device index %d\n",
                    deviceIndex);

                return m_deviceRayTracingBufferPools.at(deviceIndex);
            }

        private:
            bool m_initialized = false;
            RHI::Ptr<RHI::MultiDeviceBufferPool> m_shaderTableBufferPool;
            RHI::Ptr<RHI::MultiDeviceBufferPool> m_scratchBufferPool;
            RHI::Ptr<RHI::MultiDeviceBufferPool> m_blasBufferPool;
            RHI::Ptr<RHI::MultiDeviceBufferPool> m_tlasInstancesBufferPool;
            RHI::Ptr<RHI::MultiDeviceBufferPool> m_tlasBufferPool;

            AZStd::unordered_map<int, Ptr<RayTracingBufferPools>> m_deviceRayTracingBufferPools;
        };
    } // namespace RHI
} // namespace AZ
