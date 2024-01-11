/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceRayTracingPipelineState.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    SingleDeviceRayTracingPipelineStateDescriptor MultiDeviceRayTracingPipelineStateDescriptor::GetDeviceRayTracingPipelineStateDescriptor(
        int deviceIndex) const
    {
        AZ_Assert(m_mdPipelineState, "No MultiDevicePipelineState available\n");

        SingleDeviceRayTracingPipelineStateDescriptor descriptor{ m_descriptor };

        if (m_mdPipelineState)
        {
            descriptor.PipelineState(m_mdPipelineState->GetDevicePipelineState(deviceIndex).get());
        }

        return descriptor;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::Build()
    {
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::MaxPayloadSize(uint32_t maxPayloadSize)
    {
        m_descriptor.MaxPayloadSize(maxPayloadSize);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::MaxAttributeSize(uint32_t maxAttributeSize)
    {
        m_descriptor.MaxAttributeSize(maxAttributeSize);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::MaxRecursionDepth(
        uint32_t maxRecursionDepth)
    {
        m_descriptor.MaxRecursionDepth(maxRecursionDepth);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::PipelineState(
        const RHI::MultiDevicePipelineState* pipelineState)
    {
        m_mdPipelineState = pipelineState;
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::ShaderLibrary(
        RHI::PipelineStateDescriptorForRayTracing& descriptor)
    {
        m_descriptor.ShaderLibrary(descriptor);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::RayGenerationShaderName(
        const AZ::Name& name)
    {
        m_descriptor.RayGenerationShaderName(name);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::MissShaderName(const AZ::Name& name)
    {
        m_descriptor.MissShaderName(name);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::ClosestHitShaderName(
        const AZ::Name& closestHitShaderName)
    {
        m_descriptor.ClosestHitShaderName(closestHitShaderName);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::AnyHitShaderName(
        const AZ::Name& anyHitShaderName)
    {
        m_descriptor.AnyHitShaderName(anyHitShaderName);
        return this;
    }

    MultiDeviceRayTracingPipelineStateDescriptor* MultiDeviceRayTracingPipelineStateDescriptor::HitGroup(const AZ::Name& hitGroupName)
    {
        m_descriptor.HitGroup(hitGroupName);
        return this;
    }

    ResultCode MultiDeviceRayTracingPipelineState::Init(
        MultiDevice::DeviceMask deviceMask, const MultiDeviceRayTracingPipelineStateDescriptor& descriptor)
    {
        m_mdDescriptor = descriptor;

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode{ ResultCode::Success };

        IterateDevices(
            [this, &resultCode](int deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingPipelineState();

                auto descriptor{ m_mdDescriptor.GetDeviceRayTracingPipelineStateDescriptor(deviceIndex) };
                resultCode = GetDeviceRayTracingPipelineState(deviceIndex)->Init(*device, &descriptor);
                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific SingleDeviceRayTracingPipelineState and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    void MultiDeviceRayTracingPipelineState::Shutdown()
    {
        MultiDeviceObject::Shutdown();
    }
} // namespace AZ::RHI
