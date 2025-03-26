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
    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::Build()
    {
        return this;
    }

    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::Geometry()
    {
        m_geometries.emplace_back();
        m_buildContext = &m_geometries.back();
        return this;
    }

    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::AABB(const Aabb& aabb)
    {
        m_aabb = aabb;
        return this;
    }

    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::VertexBuffer(const RHI::DeviceStreamBufferView& vertexBuffer)
    {
        AZ_Assert(m_buildContext, "VertexBuffer property can only be added to a Geometry entry");
        m_buildContext->m_vertexBuffer = vertexBuffer;
        return this;
    }

    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::VertexFormat(RHI::Format vertexFormat)
    {
        AZ_Assert(m_buildContext, "VertexFormat property can only be added to a Geometry entry");
        m_buildContext->m_vertexFormat = vertexFormat;
        return this;
    }

    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::IndexBuffer(const RHI::DeviceIndexBufferView& indexBuffer)
    {
        AZ_Assert(m_buildContext, "IndexBuffer property can only be added to a Geometry entry");
        m_buildContext->m_indexBuffer = indexBuffer;
        return this;
    }

    DeviceRayTracingBlasDescriptor* DeviceRayTracingBlasDescriptor::BuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags)
    {
        AZ_Assert(m_buildContext || m_aabb, "BuildFlags property can only be added to a Geometry or AABB entry");
        m_buildFlags = buildFlags;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::Build()
    {
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::Instance()
    {
        AZ_Assert(m_instancesBuffer == nullptr, "Instance cannot be combined with an instances buffer");
        m_instances.emplace_back();
        m_buildContext = &m_instances.back();
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::InstanceID(uint32_t instanceID)
    {
        AZ_Assert(m_buildContext, "InstanceID property can only be added to an Instance entry");
        m_buildContext->m_instanceID = instanceID;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::InstanceMask(uint32_t instanceMask)
    {
        AZ_Assert(m_buildContext, "InstanceMask property can only be added to an Instance entry");
        m_buildContext->m_instanceMask = instanceMask;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::HitGroupIndex(uint32_t hitGroupIndex)
    {
        AZ_Assert(m_buildContext, "HitGroupIndex property can only be added to an Instance entry");
        m_buildContext->m_hitGroupIndex = hitGroupIndex;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::Transform(const AZ::Transform& transform)
    {
        AZ_Assert(m_buildContext, "Transform property can only be added to an Instance entry");
        m_buildContext->m_transform = transform;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::NonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        AZ_Assert(m_buildContext, "NonUniformSCale property can only be added to an Instance entry");
        m_buildContext->m_nonUniformScale = nonUniformScale;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::Transparent(bool transparent)
    {
        AZ_Assert(m_buildContext, "Transparent property can only be added to a Geometry entry");
        m_buildContext->m_transparent = transparent;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::Blas(const RHI::Ptr<RHI::DeviceRayTracingBlas>& blas)
    {
        AZ_Assert(m_buildContext, "Blas property can only be added to an Instance entry");
        m_buildContext->m_blas = blas;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::InstancesBuffer(const RHI::Ptr<RHI::DeviceBuffer>& instancesBuffer)
    {
        AZ_Assert(!m_buildContext, "InstancesBuffer property can only be added to the top level");
        AZ_Assert(m_instances.size() == 0, "InstancesBuffer cannot exist with instance entries");
        m_instancesBuffer = instancesBuffer;
        return this;
    }

    DeviceRayTracingTlasDescriptor* DeviceRayTracingTlasDescriptor::NumInstances(uint32_t numInstancesInBuffer)
    {
        AZ_Assert(m_instancesBuffer.get(), "NumInstances property can only be added to the InstancesBuffer entry");
        m_numInstancesInBuffer = numInstancesInBuffer;
        return this;
    }

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
            m_geometries = descriptor->GetGeometries();
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
