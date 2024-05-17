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
#include <Atom/RHI/SingleDeviceRayTracingBufferPools.h>

namespace AZ::RHI
{
    class MultiDeviceBufferPool;

    //! MultiDeviceRayTracingBufferPools
    //!
    //! This class encapsulates all of the MultiDeviceBufferPools needed for ray tracing, freeing the application
    //! from setting up and managing the buffers pools individually.
    //!
    class MultiDeviceRayTracingBufferPools : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingBufferPools, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceRayTracingBufferPools, "{59397661-F5A5-44DE-9B1D-E6F5BC32DC51}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingBufferPools);
        MultiDeviceRayTracingBufferPools() = default;
        virtual ~MultiDeviceRayTracingBufferPools() = default;

        //! Accessors
        const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetShaderTableBufferPool() const;
        const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetScratchBufferPool() const;
        const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetBlasBufferPool() const;
        const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetTlasInstancesBufferPool() const;
        const RHI::Ptr<RHI::MultiDeviceBufferPool>& GetTlasBufferPool() const;

        //! Initializes the multi-device BufferPools as well as all device-specific SingleDeviceRayTracingBufferPools
        void Init(MultiDevice::DeviceMask deviceMask);

    private:
        bool m_initialized = false;
        RHI::Ptr<RHI::MultiDeviceBufferPool> m_mdShaderTableBufferPool;
        RHI::Ptr<RHI::MultiDeviceBufferPool> m_mdScratchBufferPool;
        RHI::Ptr<RHI::MultiDeviceBufferPool> m_mdBlasBufferPool;
        RHI::Ptr<RHI::MultiDeviceBufferPool> m_mdTlasInstancesBufferPool;
        RHI::Ptr<RHI::MultiDeviceBufferPool> m_mdTlasBufferPool;
    };
} // namespace AZ::RHI
