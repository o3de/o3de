/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingCompactionQueryPool.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourcePool.h>

namespace AZ::RHI
{
    class RayTracingCompactionQueryPool;

    //! Class for querying the compacted size of a Raytracing Acceleration Structure
    //! For more information see DeviceRayTracingCompactionQuery
    class RayTracingCompactionQuery : public MultiDeviceObject
    {
        friend class RayTracingCompactionQueryPool;

    public:
        AZ_CLASS_ALLOCATOR(RayTracingCompactionQuery, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingCompactionQuery, "{2e45d8d4-4ab2-4187-b0af-53d8e4ba9b94}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingCompactionQuery);
        virtual ~RayTracingCompactionQuery() override = default;
        RayTracingCompactionQuery() = default;
    };

    //! Provides storage for DeviceRayTracingCompactionQuery objects and handles
    class RayTracingCompactionQueryPool : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingCompactionQueryPool, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingCompactionQueryPool, "{720A480B-AF13-4D28-8851-7C7D853927E0}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingCompactionQueryPool);
        RayTracingCompactionQueryPool() = default;
        virtual ~RayTracingCompactionQueryPool() override = default;

        ResultCode Init(RayTracingCompactionQueryPoolDescriptor desc);

        ResultCode InitQuery(RayTracingCompactionQuery* query);

        void BeginFrame(int frame);
    };
} // namespace AZ::RHI
