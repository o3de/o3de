/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/SingleDeviceBuffer.h>
#include <Atom/RHI/Factory.h>

namespace AZ::RHI
{
    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::Build()
    {
        return this;
    }

    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::Geometry()
    {
        m_geometries.emplace_back();
        m_buildContext = &m_geometries.back();
        return this;
    }

    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::AABB(const Aabb& aabb)
    {
        m_aabb = aabb;
        return this;
    }

    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::VertexBuffer(const RHI::SingleDeviceStreamBufferView& vertexBuffer)
    {
        AZ_Assert(m_buildContext, "VertexBuffer property can only be added to a Geometry entry");
        m_buildContext->m_vertexBuffer = vertexBuffer;
        return this;
    }

    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::VertexFormat(RHI::Format vertexFormat)
    {
        AZ_Assert(m_buildContext, "VertexFormat property can only be added to a Geometry entry");
        m_buildContext->m_vertexFormat = vertexFormat;
        return this;
    }

    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::IndexBuffer(const RHI::SingleDeviceIndexBufferView& indexBuffer)
    {
        AZ_Assert(m_buildContext, "IndexBuffer property can only be added to a Geometry entry");
        m_buildContext->m_indexBuffer = indexBuffer;
        return this;
    }

    SingleDeviceRayTracingBlasDescriptor* SingleDeviceRayTracingBlasDescriptor::BuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags)
    {
        AZ_Assert(m_buildContext, "BuildFlags property can only be added to a Geometry entry");
        m_buildFlags = buildFlags;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::Build()
    {
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::Instance()
    {
        AZ_Assert(m_instancesBuffer == nullptr, "Instance cannot be combined with an instances buffer");
        m_instances.emplace_back();
        m_buildContext = &m_instances.back();
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::InstanceID(uint32_t instanceID)
    {
        AZ_Assert(m_buildContext, "InstanceID property can only be added to an Instance entry");
        m_buildContext->m_instanceID = instanceID;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::InstanceMask(uint32_t instanceMask)
    {
        AZ_Assert(m_buildContext, "InstanceMask property can only be added to an Instance entry");
        m_buildContext->m_instanceMask = instanceMask;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::HitGroupIndex(uint32_t hitGroupIndex)
    {
        AZ_Assert(m_buildContext, "HitGroupIndex property can only be added to an Instance entry");
        m_buildContext->m_hitGroupIndex = hitGroupIndex;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::Transform(const AZ::Transform& transform)
    {
        AZ_Assert(m_buildContext, "Transform property can only be added to an Instance entry");
        m_buildContext->m_transform = transform;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::NonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        AZ_Assert(m_buildContext, "NonUniformSCale property can only be added to an Instance entry");
        m_buildContext->m_nonUniformScale = nonUniformScale;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::Transparent(bool transparent)
    {
        AZ_Assert(m_buildContext, "Transparent property can only be added to a Geometry entry");
        m_buildContext->m_transparent = transparent;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::Blas(const RHI::Ptr<RHI::SingleDeviceRayTracingBlas>& blas)
    {
        AZ_Assert(m_buildContext, "Blas property can only be added to an Instance entry");
        m_buildContext->m_blas = blas;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::InstancesBuffer(const RHI::Ptr<RHI::SingleDeviceBuffer>& instancesBuffer)
    {
        AZ_Assert(!m_buildContext, "InstancesBuffer property can only be added to the top level");
        AZ_Assert(m_instances.size() == 0, "InstancesBuffer cannot exist with instance entries");
        m_instancesBuffer = instancesBuffer;
        return this;
    }

    SingleDeviceRayTracingTlasDescriptor* SingleDeviceRayTracingTlasDescriptor::NumInstances(uint32_t numInstancesInBuffer)
    {
        AZ_Assert(m_instancesBuffer.get(), "NumInstances property can only be added to the InstancesBuffer entry");
        m_numInstancesInBuffer = numInstancesInBuffer;
        return this;
    }

    RHI::Ptr<RHI::SingleDeviceRayTracingBlas> SingleDeviceRayTracingBlas::CreateRHIRayTracingBlas()
    {
        RHI::Ptr<RHI::SingleDeviceRayTracingBlas> rayTracingBlas = RHI::Factory::Get().CreateRayTracingBlas();
        AZ_Error("SingleDeviceRayTracingBlas", rayTracingBlas.get(), "Failed to create RHI::SingleDeviceRayTracingBlas");
        return rayTracingBlas;
    }

    ResultCode SingleDeviceRayTracingBlas::CreateBuffers(Device& device, const SingleDeviceRayTracingBlasDescriptor* descriptor, const SingleDeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        ResultCode resultCode = CreateBuffersInternal(device, descriptor, rayTracingBufferPools);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
            m_geometries = descriptor->GetGeometries();
        }
        return resultCode;
    }

    RHI::Ptr<RHI::SingleDeviceRayTracingTlas> SingleDeviceRayTracingTlas::CreateRHIRayTracingTlas()
    {
        RHI::Ptr<RHI::SingleDeviceRayTracingTlas> rayTracingTlas = RHI::Factory::Get().CreateRayTracingTlas();
        AZ_Error("SingleDeviceRayTracingTlas", rayTracingTlas.get(), "Failed to create RHI::SingleDeviceRayTracingTlas");
        return rayTracingTlas;
    }

    ResultCode SingleDeviceRayTracingTlas::CreateBuffers(Device& device, const SingleDeviceRayTracingTlasDescriptor* descriptor, const SingleDeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        ResultCode resultCode = CreateBuffersInternal(device, descriptor, rayTracingBufferPools);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }
        return resultCode;
    }
}
