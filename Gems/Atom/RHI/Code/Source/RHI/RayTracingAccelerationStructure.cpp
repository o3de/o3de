/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    DeviceRayTracingBlasDescriptor RayTracingBlasDescriptor::GetDeviceRayTracingBlasDescriptor(int deviceIndex) const
    {
        DeviceRayTracingBlasDescriptor descriptor;

        for (const auto& geometry : m_geometries)
        {
            descriptor.Geometry()
                ->VertexFormat(geometry.m_vertexFormat)
                ->VertexBuffer(geometry.m_vertexBuffer.GetDeviceStreamBufferView(deviceIndex))
                ->IndexBuffer(geometry.m_indexBuffer.GetDeviceIndexBufferView(deviceIndex));
        }

        if(m_aabb.has_value())
        {
            descriptor.AABB(m_aabb.value());
        }

        descriptor.BuildFlags(m_buildFlags);

        return descriptor;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::Build()
    {
        return this;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::Geometry()
    {
        m_geometries.emplace_back();
        m_buildContext = &m_geometries.back();
        return this;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::AABB(const Aabb& aabb)
    {
        m_aabb = aabb;
        return this;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::VertexBuffer(
        const RHI::StreamBufferView& vertexBuffer)
    {
        AZ_Assert(m_buildContext, "VertexBuffer property can only be added to a Geometry entry");
        m_buildContext->m_vertexBuffer = vertexBuffer;
        return this;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::VertexFormat(RHI::Format vertexFormat)
    {
        AZ_Assert(m_buildContext, "VertexFormat property can only be added to a Geometry entry");
        m_buildContext->m_vertexFormat = vertexFormat;
        return this;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::IndexBuffer(
        const RHI::IndexBufferView& indexBuffer)
    {
        AZ_Assert(m_buildContext, "IndexBuffer property can only be added to a Geometry entry");
        m_buildContext->m_indexBuffer = indexBuffer;
        return this;
    }

    RayTracingBlasDescriptor* RayTracingBlasDescriptor::BuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags)
    {
        AZ_Assert(m_buildContext || m_aabb, "BuildFlags property can only be added to a Geometry or AABB entry");
        m_buildFlags = buildFlags;
        return this;
    }

    DeviceRayTracingTlasDescriptor RayTracingTlasDescriptor::GetDeviceRayTracingTlasDescriptor(int deviceIndex) const
    {
        DeviceRayTracingTlasDescriptor descriptor;

        for (const auto& instance : m_instances)
        {
            descriptor.Instance()
                ->InstanceID(instance.m_instanceID)
                ->HitGroupIndex(instance.m_hitGroupIndex)
                ->InstanceMask(instance.m_instanceMask)
                ->Transform(instance.m_transform)
                ->NonUniformScale(instance.m_nonUniformScale)
                ->Transparent(instance.m_transparent)
                ->Blas(instance.m_blas->GetDeviceRayTracingBlas(deviceIndex));
        }

        if (m_instancesBuffer)
        {
            descriptor.InstancesBuffer(m_instancesBuffer->GetDeviceBuffer(deviceIndex))->NumInstances(m_numInstancesInBuffer);
        }

        return descriptor;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::Build()
    {
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::Instance()
    {
        AZ_Assert(m_instancesBuffer == nullptr, "Instance cannot be combined with an instances buffer");
        m_instances.emplace_back();
        m_buildContext = &m_instances.back();
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::InstanceID(uint32_t instanceID)
    {
        AZ_Assert(m_buildContext, "InstanceID property can only be added to an Instance entry");
        m_buildContext->m_instanceID = instanceID;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::InstanceMask(uint32_t instanceMask)
    {
        AZ_Assert(m_buildContext, "InstanceMask property can only be added to an Instance entry");
        m_buildContext->m_instanceMask = instanceMask;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::HitGroupIndex(uint32_t hitGroupIndex)
    {
        AZ_Assert(m_buildContext, "HitGroupIndex property can only be added to an Instance entry");
        m_buildContext->m_hitGroupIndex = hitGroupIndex;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::Transform(const AZ::Transform& transform)
    {
        AZ_Assert(m_buildContext, "Transform property can only be added to an Instance entry");
        m_buildContext->m_transform = transform;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::NonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        AZ_Assert(m_buildContext, "NonUniformSCale property can only be added to an Instance entry");
        m_buildContext->m_nonUniformScale = nonUniformScale;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::Transparent(bool transparent)
    {
        AZ_Assert(m_buildContext, "Transparent property can only be added to a Geometry entry");
        m_buildContext->m_transparent = transparent;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::Blas(const RHI::Ptr<RHI::RayTracingBlas>& blas)
    {
        AZ_Assert(m_buildContext, "Blas property can only be added to an Instance entry");
        m_buildContext->m_blas = blas;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::InstancesBuffer(
        RHI::Ptr<RHI::Buffer>& instancesBuffer)
    {
        AZ_Assert(!m_buildContext, "InstancesBuffer property can only be added to the top level");
        AZ_Assert(m_instances.size() == 0, "InstancesBuffer cannot exist with instance entries");
        m_instancesBuffer = instancesBuffer;
        return this;
    }

    RayTracingTlasDescriptor* RayTracingTlasDescriptor::NumInstances(uint32_t numInstancesInBuffer)
    {
        AZ_Assert(m_instancesBuffer.get(), "NumInstances property can only be added to the InstancesBuffer entry");
        m_numInstancesInBuffer = numInstancesInBuffer;
        return this;
    }

    ResultCode RayTracingBlas::CreateBuffers(
        MultiDevice::DeviceMask deviceMask,
        const RayTracingBlasDescriptor* descriptor,
        const RayTracingBufferPools& rayTracingBufferPools)
    {
        m_descriptor = *descriptor;
        ResultCode resultCode{ ResultCode::Success };

        MultiDeviceObject::Init(deviceMask);

        IterateDevices(
            [this, &resultCode, &descriptor, &rayTracingBufferPools](auto deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                this->m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingBlas();

                auto deviceDescriptor{ descriptor->GetDeviceRayTracingBlasDescriptor(deviceIndex) };

                resultCode = GetDeviceRayTracingBlas(deviceIndex)
                                 ->CreateBuffers(
                                     *device, &deviceDescriptor, *rayTracingBufferPools.GetDeviceRayTracingBufferPools(deviceIndex).get());

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific DeviceRayTracingBlas and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        return resultCode;
    }

    ResultCode RayTracingBlas::CreateCompactedBuffers(
        const RayTracingBlas& sourceBlas,
        const AZStd::unordered_map<int, uint64_t>& compactedSizes,
        const RayTracingBufferPools& rayTracingBufferPools)
    {
        m_descriptor = sourceBlas.m_descriptor;

        MultiDeviceObject::Init(sourceBlas.GetDeviceMask());
        ResultCode resultCode{ ResultCode::Success };

        IterateDevices(
            [&](auto deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                this->m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingBlas();

                auto deviceDescriptor{ m_descriptor.GetDeviceRayTracingBlasDescriptor(deviceIndex) };

                resultCode = GetDeviceRayTracingBlas(deviceIndex)
                                 ->CreateCompactedBuffers(
                                     *device,
                                     sourceBlas.GetDeviceRayTracingBlas(deviceIndex),
                                     compactedSizes.at(deviceIndex),
                                     *rayTracingBufferPools.GetDeviceRayTracingBufferPools(deviceIndex).get());

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific DeviceRayTracingBlas and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        return resultCode;
    }

    bool RayTracingBlas::IsValid() const
    {
        if (m_deviceObjects.empty())
        {
            return false;
        }

        IterateObjects<DeviceRayTracingBlas>(
            [](auto /*deviceIndex*/, auto deviceRayTracingBlas)
            {
                if (!deviceRayTracingBlas->IsValid())
                {
                    return false;
                }
                return true;
            });
        return true;
    }

    ResultCode RayTracingTlas::CreateBuffers(
        MultiDevice::DeviceMask deviceMask,
        const RayTracingTlasDescriptor* descriptor,
        const RayTracingBufferPools& rayTracingBufferPools)
    {
        m_descriptor = *descriptor;

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode{ResultCode::Success};
        IterateDevices(
            [this, &descriptor, &rayTracingBufferPools, &resultCode](int deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                auto deviceRayTracingTlas{Factory::Get().CreateRayTracingTlas()};
                this->m_deviceObjects[deviceIndex] = deviceRayTracingTlas;

                auto deviceDescriptor{ descriptor->GetDeviceRayTracingTlasDescriptor(deviceIndex) };

                resultCode = deviceRayTracingTlas->CreateBuffers(
                    *device, &deviceDescriptor, *rayTracingBufferPools.GetDeviceRayTracingBufferPools(deviceIndex).get());
                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific DeviceRayTracingTlas and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        // Each call to CreateBuffers advances m_currentBufferIndex internally, reset buffers to always receive currently active
        m_tlasBuffer.reset();
        m_tlasInstancesBuffer.reset();

        return resultCode;
    }

    const RHI::Ptr<RHI::Buffer> RayTracingTlas::GetTlasBuffer() const
    {
        AZStd::lock_guard lock(m_tlasBufferMutex);
        if (m_deviceObjects.empty())
        {
            return nullptr;
        }

        if (m_tlasBuffer)
        {
            return m_tlasBuffer;
        }

        m_tlasBuffer = aznew RHI::Buffer;
        m_tlasBuffer->Init(GetDeviceMask());

        IterateObjects<DeviceRayTracingTlas>(
            [this](int deviceIndex, auto deviceRayTracingTlas)
            {
                m_tlasBuffer->m_deviceObjects[deviceIndex] = deviceRayTracingTlas->GetTlasBuffer();

                if (!m_tlasBuffer->m_deviceObjects[deviceIndex])
                {
                    m_tlasBuffer->m_deviceObjects.clear();
                    m_tlasBuffer = nullptr;
                    return ResultCode::Fail;
                }

                m_tlasBuffer->SetDescriptor(m_tlasBuffer->GetDeviceBuffer(deviceIndex)->GetDescriptor());
                return ResultCode::Success;
            });

        if (m_tlasBuffer)
        {
            if (const auto& name = m_tlasBuffer->GetName(); !name.IsEmpty())
            {
                m_tlasBuffer->SetName(name);
            }
        }

        return m_tlasBuffer;
    }

    const RHI::Ptr<RHI::Buffer> RayTracingTlas::GetTlasInstancesBuffer() const
    {
        AZStd::lock_guard lock(m_tlasInstancesBufferMutex);

        if (m_deviceObjects.empty())
        {
            return nullptr;
        }

        if (m_tlasInstancesBuffer)
        {
            return m_tlasInstancesBuffer;
        }

        m_tlasInstancesBuffer = aznew RHI::Buffer;
        m_tlasInstancesBuffer->Init(GetDeviceMask());

        IterateObjects<DeviceRayTracingTlas>(
            [this](int deviceIndex, auto deviceRayTracingTlas)
            {
                m_tlasInstancesBuffer->m_deviceObjects[deviceIndex] = deviceRayTracingTlas->GetTlasBuffer();

                if (!m_tlasInstancesBuffer->m_deviceObjects[deviceIndex])
                {
                    m_tlasInstancesBuffer->m_deviceObjects.clear();
                    m_tlasInstancesBuffer = nullptr;
                    return ResultCode::Fail;
                }

                m_tlasInstancesBuffer->SetDescriptor(m_tlasInstancesBuffer->GetDeviceBuffer(deviceIndex)->GetDescriptor());
                return ResultCode::Success;
            });

        if (m_tlasInstancesBuffer)
        {
            if (const auto& name = m_tlasInstancesBuffer->GetName(); !name.IsEmpty())
            {
                m_tlasInstancesBuffer->SetName(name);
            }
        }

        return m_tlasInstancesBuffer;
    }
} // namespace AZ::RHI
