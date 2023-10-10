/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/MultiDeviceRayTracingBufferPools.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    RayTracingBlasDescriptor MultiDeviceRayTracingBlasDescriptor::GetDeviceRayTracingBlasDescriptor(int deviceIndex) const
    {
        RayTracingBlasDescriptor descriptor;

        for (const auto& geometry : m_mdGeometries)
        {
            descriptor.Geometry()
                ->VertexFormat(geometry.m_vertexFormat)
                ->VertexBuffer(geometry.m_mdVertexBuffer.GetDeviceStreamBufferView(deviceIndex))
                ->IndexBuffer(geometry.m_mdIndexBuffer.GetDeviceIndexBufferView(deviceIndex));
        }

        return descriptor;
    }

    MultiDeviceRayTracingBlasDescriptor* MultiDeviceRayTracingBlasDescriptor::Build()
    {
        return this;
    }

    MultiDeviceRayTracingBlasDescriptor* MultiDeviceRayTracingBlasDescriptor::Geometry()
    {
        m_mdGeometries.emplace_back();
        m_mdBuildContext = &m_mdGeometries.back();
        return this;
    }

    MultiDeviceRayTracingBlasDescriptor* MultiDeviceRayTracingBlasDescriptor::VertexBuffer(
        const RHI::MultiDeviceStreamBufferView& vertexBuffer)
    {
        AZ_Assert(m_mdBuildContext, "VertexBuffer property can only be added to a Geometry entry");
        m_mdBuildContext->m_mdVertexBuffer = vertexBuffer;
        return this;
    }

    MultiDeviceRayTracingBlasDescriptor* MultiDeviceRayTracingBlasDescriptor::VertexFormat(RHI::Format vertexFormat)
    {
        AZ_Assert(m_mdBuildContext, "VertexFormat property can only be added to a Geometry entry");
        m_mdBuildContext->m_vertexFormat = vertexFormat;
        return this;
    }

    MultiDeviceRayTracingBlasDescriptor* MultiDeviceRayTracingBlasDescriptor::IndexBuffer(
        const RHI::MultiDeviceIndexBufferView& indexBuffer)
    {
        AZ_Assert(m_mdBuildContext, "IndexBuffer property can only be added to a Geometry entry");
        m_mdBuildContext->m_mdIndexBuffer = indexBuffer;
        return this;
    }

    RayTracingTlasDescriptor MultiDeviceRayTracingTlasDescriptor::GetDeviceRayTracingTlasDescriptor(int deviceIndex) const
    {
        AZ_Assert(m_mdInstancesBuffer, "No MultiDeviceBuffer available!\n");

        RayTracingTlasDescriptor descriptor;

        for (const auto& instance : m_mdInstances)
        {
            descriptor.Instance()
                ->InstanceID(instance.m_instanceID)
                ->HitGroupIndex(instance.m_hitGroupIndex)
                ->Transform(instance.m_transform)
                ->NonUniformScale(instance.m_nonUniformScale)
                ->Transparent(instance.m_transparent)
                ->Blas(instance.m_mdBlas->GetDeviceRayTracingBlas(deviceIndex));
        }

        if (m_mdInstancesBuffer)
        {
            descriptor.InstancesBuffer(m_mdInstancesBuffer->GetDeviceBuffer(deviceIndex))->NumInstances(m_numInstancesInBuffer);
        }

        return descriptor;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::Build()
    {
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::Instance()
    {
        AZ_Assert(m_mdInstancesBuffer == nullptr, "Instance cannot be combined with an instances buffer");
        m_mdInstances.emplace_back();
        m_mdBuildContext = &m_mdInstances.back();
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::InstanceID(uint32_t instanceID)
    {
        AZ_Assert(m_mdBuildContext, "InstanceID property can only be added to an Instance entry");
        m_mdBuildContext->m_instanceID = instanceID;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::HitGroupIndex(uint32_t hitGroupIndex)
    {
        AZ_Assert(m_mdBuildContext, "HitGroupIndex property can only be added to an Instance entry");
        m_mdBuildContext->m_hitGroupIndex = hitGroupIndex;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::Transform(const AZ::Transform& transform)
    {
        AZ_Assert(m_mdBuildContext, "Transform property can only be added to an Instance entry");
        m_mdBuildContext->m_transform = transform;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::NonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        AZ_Assert(m_mdBuildContext, "NonUniformSCale property can only be added to an Instance entry");
        m_mdBuildContext->m_nonUniformScale = nonUniformScale;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::Transparent(bool transparent)
    {
        AZ_Assert(m_mdBuildContext, "Transparent property can only be added to a Geometry entry");
        m_mdBuildContext->m_transparent = transparent;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::Blas(RHI::Ptr<RHI::MultiDeviceRayTracingBlas>& blas)
    {
        AZ_Assert(m_mdBuildContext, "Blas property can only be added to an Instance entry");
        m_mdBuildContext->m_mdBlas = blas;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::InstancesBuffer(
        RHI::Ptr<RHI::MultiDeviceBuffer>& instancesBuffer)
    {
        AZ_Assert(!m_mdBuildContext, "InstancesBuffer property can only be added to the top level");
        AZ_Assert(m_mdInstances.size() == 0, "InstancesBuffer cannot exist with instance entries");
        m_mdInstancesBuffer = instancesBuffer;
        return this;
    }

    MultiDeviceRayTracingTlasDescriptor* MultiDeviceRayTracingTlasDescriptor::NumInstances(uint32_t numInstancesInBuffer)
    {
        AZ_Assert(m_mdInstancesBuffer.get(), "NumInstances property can only be added to the InstancesBuffer entry");
        m_numInstancesInBuffer = numInstancesInBuffer;
        return this;
    }

    ResultCode MultiDeviceRayTracingBlas::CreateBuffers(
        MultiDevice::DeviceMask deviceMask,
        const MultiDeviceRayTracingBlasDescriptor* descriptor,
        const MultiDeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        m_mdDescriptor = *descriptor;
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
            // Reset already initialized device-specific RayTracingBlas and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    bool MultiDeviceRayTracingBlas::IsValid() const
    {
        if (m_deviceObjects.empty())
        {
            return false;
        }

        IterateObjects<RayTracingBlas>(
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

    ResultCode MultiDeviceRayTracingTlas::CreateBuffers(
        MultiDevice::DeviceMask deviceMask,
        const MultiDeviceRayTracingTlasDescriptor* descriptor,
        const MultiDeviceRayTracingBufferPools& rayTracingBufferPools)
    {
        m_mdDescriptor = *descriptor;
        ResultCode resultCode{ ResultCode::Success };

        MultiDeviceObject::Init(deviceMask);

        IterateObjects<RayTracingTlas>(
            [this, &resultCode, &descriptor, &rayTracingBufferPools](int deviceIndex, auto deviceRayTracingTlas)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                this->m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingTlas();

                auto deviceDescriptor{ descriptor->GetDeviceRayTracingTlasDescriptor(deviceIndex) };

                resultCode = deviceRayTracingTlas->CreateBuffers(
                    *device, &deviceDescriptor, *rayTracingBufferPools.GetDeviceRayTracingBufferPools(deviceIndex).get());

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific RayTracingTlas and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    const RHI::Ptr<RHI::MultiDeviceBuffer> MultiDeviceRayTracingTlas::GetTlasBuffer() const
    {
        if (m_deviceObjects.empty())
        {
            return nullptr;
        }

        auto tlasBuffer = aznew RHI::MultiDeviceBuffer;
        tlasBuffer->Init(GetDeviceMask());

        IterateObjects<RayTracingTlas>(
            [&tlasBuffer](int deviceIndex, auto deviceRayTracingTlas)
            {
                tlasBuffer->m_deviceObjects[deviceIndex] = deviceRayTracingTlas->GetTlasBuffer();

                if (!tlasBuffer->m_deviceObjects[deviceIndex])
                {
                    tlasBuffer = nullptr;
                    return false;
                }

                tlasBuffer->SetDescriptor(tlasBuffer->GetDeviceBuffer(deviceIndex)->GetDescriptor());
                return true;
            });

        return tlasBuffer;
    }

    const RHI::Ptr<RHI::MultiDeviceBuffer> MultiDeviceRayTracingTlas::GetTlasInstancesBuffer() const
    {
        if (m_deviceObjects.empty())
        {
            return nullptr;
        }

        auto tlasInstancesBuffer = aznew RHI::MultiDeviceBuffer;
        tlasInstancesBuffer->Init(GetDeviceMask());

        IterateObjects<RayTracingTlas>(
            [&tlasInstancesBuffer](int deviceIndex, auto deviceRayTracingTlas)
            {
                tlasInstancesBuffer->m_deviceObjects[deviceIndex] = deviceRayTracingTlas->GetTlasBuffer();

                if (!tlasInstancesBuffer->m_deviceObjects[deviceIndex])
                {
                    tlasInstancesBuffer = nullptr;
                    return false;
                }

                tlasInstancesBuffer->SetDescriptor(tlasInstancesBuffer->GetDeviceBuffer(deviceIndex)->GetDescriptor());
                return true;
            });
        return tlasInstancesBuffer;
    }
} // namespace AZ::RHI
