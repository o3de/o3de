/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace RHI
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

        RayTracingBlasDescriptor* RayTracingBlasDescriptor::VertexBuffer(const RHI::StreamBufferView& vertexBuffer)
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

        RayTracingBlasDescriptor* RayTracingBlasDescriptor::IndexBuffer(const RHI::IndexBufferView& indexBuffer)
        {
            AZ_Assert(m_buildContext, "IndexBuffer property can only be added to a Geometry entry");
            m_buildContext->m_indexBuffer = indexBuffer;
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
                    ->Transform(instance.m_transform)
                    ->NonUniformScale(instance.m_nonUniformScale)
                    ->Transparent(instance.m_transparent)
                    ->Blas(instance.m_blas->GetDeviceRayTracingBlas(deviceIndex));
            }

            if (m_instancesBuffer)
                descriptor.InstancesBuffer(m_instancesBuffer->GetDeviceBuffer(deviceIndex))->NumInstances(m_numInstancesInBuffer);

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

        RayTracingTlasDescriptor* RayTracingTlasDescriptor::Blas(RHI::Ptr<RHI::RayTracingBlas>& blas)
        {
            AZ_Assert(m_buildContext, "Blas property can only be added to an Instance entry");
            m_buildContext->m_blas = blas;
            return this;
        }

        RayTracingTlasDescriptor* RayTracingTlasDescriptor::InstancesBuffer(RHI::Ptr<RHI::Buffer>& instancesBuffer)
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
            DeviceMask deviceMask, const RayTracingBlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools)
        {
            m_descriptor = *descriptor;
            ResultCode resultCode{ ResultCode::Success };

            MultiDeviceObject::Init(deviceMask);

            IterateDevices(
                [this, &resultCode, &descriptor, &rayTracingBufferPools](int deviceIndex)
                {
                    auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                    m_deviceRayTracingBlas[deviceIndex] = Factory::Get().CreateRayTracingBlas();

                    auto deviceDescriptor{ descriptor->GetDeviceRayTracingBlasDescriptor(deviceIndex) };

                    resultCode = m_deviceRayTracingBlas[deviceIndex]->CreateBuffers(
                        *device, &deviceDescriptor, *rayTracingBufferPools.GetDeviceRayTracingBufferPool(deviceIndex).get());

                    return resultCode == ResultCode::Success;
                });

            return resultCode;
        }

        bool RayTracingBlas::IsValid() const
        {
            if (m_deviceRayTracingBlas.empty())
                return false;

            for (const auto& [deviceIndex, blas] : m_deviceRayTracingBlas)
            {
                if (!blas->IsValid())
                    return false;
            }
            return true;
        }

        ResultCode RayTracingTlas::CreateBuffers(
            DeviceMask deviceMask, const RayTracingTlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools)
        {
            m_descriptor = *descriptor;
            ResultCode resultCode{ ResultCode::Success };

            MultiDeviceObject::Init(deviceMask);

            IterateDevices(
                [this, &resultCode, &descriptor, &rayTracingBufferPools](int deviceIndex)
                {
                    auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                    m_deviceRayTracingTlas[deviceIndex] = Factory::Get().CreateRayTracingTlas();

                    DeviceRayTracingTlasDescriptor deviceDescriptor{ descriptor->GetDeviceRayTracingTlasDescriptor(deviceIndex) };

                    resultCode = m_deviceRayTracingTlas[deviceIndex]->CreateBuffers(
                        *device, &deviceDescriptor, *rayTracingBufferPools.GetDeviceRayTracingBufferPool(deviceIndex).get());

                    return resultCode == ResultCode::Success;
                });

            return resultCode;
        }

        const RHI::Ptr<RHI::Buffer> RayTracingTlas::GetTlasBuffer() const
        {
            if (m_deviceRayTracingTlas.empty())
                return nullptr;

            auto tlasBuffer = aznew RHI::Buffer;
            tlasBuffer->Init(GetDeviceMask());

            for (const auto& [deviceIndex, tlas] : m_deviceRayTracingTlas)
            {
                tlasBuffer->m_deviceBuffers[deviceIndex] = tlas->GetTlasBuffer();

                if (!tlasBuffer->m_deviceBuffers[deviceIndex])
                    return nullptr;

                tlasBuffer->SetDescriptor(tlasBuffer->m_deviceBuffers[deviceIndex]->GetDescriptor());
            }
            return tlasBuffer;
        }

        const RHI::Ptr<RHI::Buffer> RayTracingTlas::GetTlasInstancesBuffer() const
        {
            if (m_deviceRayTracingTlas.empty())
                return nullptr;

            auto tlasInstancesBuffer = aznew RHI::Buffer;
            tlasInstancesBuffer->Init(GetDeviceMask());

            for (const auto& [deviceIndex, tlas] : m_deviceRayTracingTlas)
            {
                tlasInstancesBuffer->m_deviceBuffers[deviceIndex] = tlas->GetTlasBuffer();

                if (!tlasInstancesBuffer->m_deviceBuffers[deviceIndex])
                    return nullptr;

                tlasInstancesBuffer->SetDescriptor(tlasInstancesBuffer->m_deviceBuffers[deviceIndex]->GetDescriptor());
            }
            return tlasInstancesBuffer;
        }
    } // namespace RHI
} // namespace AZ
