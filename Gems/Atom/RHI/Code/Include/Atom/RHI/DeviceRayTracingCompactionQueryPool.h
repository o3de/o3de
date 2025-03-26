/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/Resource.h>

namespace AZ::RHI
{
    class DeviceRayTracingCompactionQueryPool;

    //! Class for querying the compacted size of a Raytracing Acceleration Structure
    //!
    //! It can be used to compact acceleration structures to save memory in the raytracing scene
    //! Acceleration structure compaction is done by performing these steps:
    //!
    //! 1. Created and build the uncompacted acceleration structure
    //! 2. Query the compacted size using DeviceRayTracingCompactionQuery and wait for it to be available on the CPU
    //! 3. Create a new compacted acceleration structure with a buffer size returned by DeviceRayTracingCompactionQuery
    //! 4. Copy the uncompacted acceleration structure to the compacted acceleration structure
    //! 5. Delete the uncompacted acceleration structure to save memory
    //!
    //! This process takes multiple frames to complete as the compact size must be available on the CPU
    //!
    //! See https://developer.nvidia.com/blog/tips-acceleration-structure-compaction/ for a more detailed description
    class DeviceRayTracingCompactionQuery : public DeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(DeviceRayTracingCompactionQuery, AZ::SystemAllocator, 0);
        AZ_RTTI(DeviceRayTracingCompactionQuery, "{9f01df87-c773-4e9c-bdfd-93331ddbfdaf}", DeviceObject);
        virtual ~DeviceRayTracingCompactionQuery() override = default;
        DeviceRayTracingCompactionQuery() = default;

        ResultCode Init(Device& device, DeviceRayTracingCompactionQueryPool* pool);

        DeviceRayTracingCompactionQueryPool* GetPool();

        virtual uint64_t GetResult() = 0;

    protected:
        virtual ResultCode InitInternal(DeviceRayTracingCompactionQueryPool* pool) = 0;

        DeviceRayTracingCompactionQueryPool* m_pool = nullptr;
    };

    struct RayTracingCompactionQueryPoolDescriptor
    {
        MultiDevice::DeviceMask m_deviceMask = MultiDevice::NoDevices;
        // Number of queries in the pool
        int m_budget = -1;

        RHI::Ptr<BufferPool> m_readbackBufferPool;
        RHI::Ptr<BufferPool> m_copyBufferPool;
    };

    //! Provides storage for DeviceRayTracingCompactionQuery objects and handles
    class DeviceRayTracingCompactionQueryPool : public DeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(DeviceRayTracingCompactionQueryPool, AZ::SystemAllocator, 0);
        AZ_RTTI(DeviceRayTracingCompactionQueryPool, "{a6b9096c-f5be-4be9-9480-485408afb358}", DeviceObject);
        DeviceRayTracingCompactionQueryPool() = default;
        virtual ~DeviceRayTracingCompactionQueryPool() override = default;

        ResultCode Init(Device& device, RayTracingCompactionQueryPoolDescriptor desc);

        const RayTracingCompactionQueryPoolDescriptor& GetDescriptor();

        virtual void BeginFrame([[maybe_unused]] int frame){};

    protected:
        virtual ResultCode InitInternal(RayTracingCompactionQueryPoolDescriptor desc) = 0;

        RayTracingCompactionQueryPoolDescriptor m_desc;
    };
} // namespace AZ::RHI
