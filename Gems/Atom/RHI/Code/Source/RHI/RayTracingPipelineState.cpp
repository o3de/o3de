/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    DeviceRayTracingPipelineStateDescriptor RayTracingPipelineStateDescriptor::GetDeviceRayTracingPipelineStateDescriptor(
        int deviceIndex) const
    {
        AZ_Assert(m_pipelineState, "No PipelineState available\n");

        DeviceRayTracingPipelineStateDescriptor descriptor{ m_descriptor };

        if (m_pipelineState)
        {
            descriptor.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();
        }

        return descriptor;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::Build()
    {
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxPayloadSize(uint32_t maxPayloadSize)
    {
        m_descriptor.MaxPayloadSize(maxPayloadSize);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxAttributeSize(uint32_t maxAttributeSize)
    {
        m_descriptor.MaxAttributeSize(maxAttributeSize);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxRecursionDepth(
        uint32_t maxRecursionDepth)
    {
        m_descriptor.MaxRecursionDepth(maxRecursionDepth);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::PipelineState(
        const RHI::PipelineState* pipelineState)
    {
        m_pipelineState = pipelineState;
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ShaderLibrary(
        RHI::PipelineStateDescriptorForRayTracing& descriptor)
    {
        m_descriptor.ShaderLibrary(descriptor);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::RayGenerationShaderName(
        const AZ::Name& name)
    {
        m_descriptor.RayGenerationShaderName(name);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MissShaderName(const AZ::Name& name)
    {
        m_descriptor.MissShaderName(name);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ClosestHitShaderName(
        const AZ::Name& closestHitShaderName)
    {
        m_descriptor.ClosestHitShaderName(closestHitShaderName);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::AnyHitShaderName(
        const AZ::Name& anyHitShaderName)
    {
        m_descriptor.AnyHitShaderName(anyHitShaderName);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::IntersectionShaderName(
        const Name& intersectionShaderName)
    {
        m_descriptor.IntersectionShaderName(intersectionShaderName);
        return this;
    }

    RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::HitGroup(const AZ::Name& hitGroupName)
    {
        m_descriptor.HitGroup(hitGroupName);
        return this;
    }

    ResultCode RayTracingPipelineState::Init(
        MultiDevice::DeviceMask deviceMask, const RayTracingPipelineStateDescriptor& descriptor)
    {
        m_descriptor = descriptor;

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode{ ResultCode::Success };

        IterateDevices(
            [this, &resultCode](int deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingPipelineState();

                auto descriptor{ m_descriptor.GetDeviceRayTracingPipelineStateDescriptor(deviceIndex) };
                resultCode = GetDeviceRayTracingPipelineState(deviceIndex)->Init(*device, &descriptor);
                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific DeviceRayTracingPipelineState and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        return resultCode;
    }

    void RayTracingPipelineState::Shutdown()
    {
        MultiDeviceObject::Shutdown();
    }
} // namespace AZ::RHI
