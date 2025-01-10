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
        int m_budget = -1;

        RHI::Ptr<BufferPool> m_readbackBufferPool;
        RHI::Ptr<BufferPool> m_copyBufferPool;
    };

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
