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

        DeviceRayTracingPipelineStateDescriptor descriptor;

        descriptor.m_configuration = m_configuration;
        descriptor.m_shaderLibraries = m_shaderLibraries;
        descriptor.m_hitGroups = m_hitGroups;

        if (m_pipelineState)
        {
            descriptor.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();
        }

        return descriptor;
    }

    void RayTracingPipelineStateDescriptor::AddRayGenerationShaderLibrary(
        const PipelineStateDescriptorForRayTracing& descriptor, const Name& rayGenerationShaderName)
    {
        auto& shaderLibrary = m_shaderLibraries.emplace_back();
        shaderLibrary.m_descriptor = descriptor;
        shaderLibrary.m_rayGenerationShaderName = rayGenerationShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddMissShaderLibrary(
        const PipelineStateDescriptorForRayTracing& descriptor, const Name& missShaderName)
    {
        auto& shaderLibrary = m_shaderLibraries.emplace_back();
        shaderLibrary.m_descriptor = descriptor;
        shaderLibrary.m_missShaderName = missShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddCallableShaderLibrary(
        const PipelineStateDescriptorForRayTracing& descriptor, const Name& callableShaderName)
    {
        auto& shaderLibrary = m_shaderLibraries.emplace_back();
        shaderLibrary.m_descriptor = descriptor;
        shaderLibrary.m_callableShaderName = callableShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddClosestHitShaderLibrary(
        const PipelineStateDescriptorForRayTracing& descriptor, const Name& closestHitShaderName)
    {
        auto& shaderLibrary = m_shaderLibraries.emplace_back();
        shaderLibrary.m_descriptor = descriptor;
        shaderLibrary.m_closestHitShaderName = closestHitShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddAnyHitShaderLibrary(
        const PipelineStateDescriptorForRayTracing& descriptor, const Name& anyHitShaderName)
    {
        auto& shaderLibrary = m_shaderLibraries.emplace_back();
        shaderLibrary.m_descriptor = descriptor;
        shaderLibrary.m_anyHitShaderName = anyHitShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddIntersectionShaderLibrary(
        const PipelineStateDescriptorForRayTracing& descriptor, const Name& intersectionShaderName)
    {
        auto& shaderLibrary = m_shaderLibraries.emplace_back();
        shaderLibrary.m_descriptor = descriptor;
        shaderLibrary.m_intersectionShaderName = intersectionShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddHitGroup(const Name& hitGroupName, const Name& closestHitShaderName)
    {
        auto& hitGroup = m_hitGroups.emplace_back();
        hitGroup.m_hitGroupName = hitGroupName;
        hitGroup.m_closestHitShaderName = closestHitShaderName;
    }

    void RayTracingPipelineStateDescriptor::AddHitGroup(
        const Name& hitGroupName, const Name& closestHitShaderName, const Name& intersectionShaderName)
    {
        auto& hitGroup = m_hitGroups.emplace_back();
        hitGroup.m_hitGroupName = hitGroupName;
        hitGroup.m_closestHitShaderName = closestHitShaderName;
        hitGroup.m_intersectionShaderName = intersectionShaderName;
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
