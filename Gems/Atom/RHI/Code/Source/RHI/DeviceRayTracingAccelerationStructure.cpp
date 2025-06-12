/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/Factory.h>

namespace AZ::RHI
{
    RHI::Ptr<RHI::DeviceRayTracingBlas> DeviceRayTracingBlas::CreateRHIRayTracingBlas()
    {
        RHI::Ptr<RHI::DeviceRayTracingBlas> rayTracingBlas = RHI::Factory::Get().CreateRayTracingBlas();
        AZ_Error("DeviceRayTracingBlas", rayTracingBlas.get(), "Failed to create RHI::DeviceRayTracingBlas");
        return rayTracingBlas;
    }

    ResultCode DeviceRayTracingBlas::CreateCompactedBuffers(
        Device& device,
        RHI::Ptr<RHI::DeviceRayTracingBlas> sourceBlas,
        uint64_t compactedBufferSize,
        const DeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        ResultCode resultCode = CreateCompactedBuffersInternal(device, sourceBlas, compactedBufferSize, rayTracingBufferPools);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
            m_geometries = sourceBlas->m_geometries;
        }
        return resultCode;
    }

    ResultCode DeviceRayTracingBlas::CreateBuffers(Device& device, const DeviceRayTracingBlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        ResultCode resultCode = CreateBuffersInternal(device, descriptor, rayTracingBufferPools);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
            m_geometries = descriptor->m_geometries;
        }
        return resultCode;
    }

    RHI::Ptr<RHI::DeviceRayTracingTlas> DeviceRayTracingTlas::CreateRHIRayTracingTlas()
    {
        RHI::Ptr<RHI::DeviceRayTracingTlas> rayTracingTlas = RHI::Factory::Get().CreateRayTracingTlas();
        AZ_Error("DeviceRayTracingTlas", rayTracingTlas.get(), "Failed to create RHI::DeviceRayTracingTlas");
        return rayTracingTlas;
    }

    ResultCode DeviceRayTracingTlas::CreateBuffers(Device& device, const DeviceRayTracingTlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        ResultCode resultCode = CreateBuffersInternal(device, descriptor, rayTracingBufferPools);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }
        return resultCode;
    }
}
