/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>

namespace AZ::RHI
{
    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::Build()
    {
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::MaxPayloadSize(uint32_t maxPayloadSize)
    {
        AZ_Assert(IsTopLevelBuildContext(), "MaxPayloadSize can only be added to the top level of the DeviceRayTracingPipelineState");
        m_configuration.m_maxPayloadSize = maxPayloadSize;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::MaxAttributeSize(uint32_t maxAttributeSize)
    {
        AZ_Assert(IsTopLevelBuildContext(), "MaxAttributeSize can only be added to the top level of the DeviceRayTracingPipelineState");
        m_configuration.m_maxAttributeSize = maxAttributeSize;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::MaxRecursionDepth(uint32_t maxRecursionDepth)
    {
        AZ_Assert(IsTopLevelBuildContext(), "MaxRecursionDepth can only be added to the top level of the DeviceRayTracingPipelineState");
        m_configuration.m_maxRecursionDepth = maxRecursionDepth;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::PipelineState(const RHI::DevicePipelineState* pipelineState)
    {
        AZ_Assert(IsTopLevelBuildContext(), "DevicePipelineState can only be added to the top level of the DeviceRayTracingPipelineState");
        m_pipelineState = pipelineState;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor)
    {
        ClearBuildContext();

        m_shaderLibraries.push_back({descriptor});
        m_shaderLibraryBuildContext = &m_shaderLibraries.back();
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::RayGenerationShaderName(const AZ::Name& name)
    {
        AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "RayGenerationShaderName can only be added to a ShaderLibrary");
        m_shaderLibraryBuildContext->m_rayGenerationShaderName = name;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::MissShaderName(const AZ::Name& name)
    {
        AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "MissShaderName can only be added to a ShaderLibrary");
        m_shaderLibraryBuildContext->m_missShaderName = name;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::CallableShaderName(const AZ::Name& callableShaderName)
    {
        AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "CallableShaderName can only be added to a ShaderLibrary");
        m_shaderLibraryBuildContext->m_callableShaderName = callableShaderName;
        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::ClosestHitShaderName(const AZ::Name& closestHitShaderName)
    {
        AZ_Assert(m_shaderLibraryBuildContext || m_hitGroupBuildContext, "ClosestHitShaderName can only be added to a ShaderLibrary or a HitGroup");

        if (m_hitGroupBuildContext)
        {
            m_hitGroupBuildContext->m_closestHitShaderName = closestHitShaderName;
        }
        else
        {
            m_shaderLibraryBuildContext->m_closestHitShaderName = closestHitShaderName;
        }

        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::AnyHitShaderName(const AZ::Name& anyHitShaderName)
    {
        AZ_Assert(m_shaderLibraryBuildContext || m_hitGroupBuildContext, "AnyHitShaderName can only be added to a ShaderLibrary or a HitGroup");

        if (m_hitGroupBuildContext)
        {
            m_hitGroupBuildContext->m_anyHitShaderName = anyHitShaderName;
        }
        else
        {
            m_shaderLibraryBuildContext->m_anyHitShaderName = anyHitShaderName;
        }

        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::IntersectionShaderName(const Name& intersectionShaderName)
    {
        AZ_Assert(m_shaderLibraryBuildContext || m_hitGroupBuildContext, "IntersectionShaderName can only be added to a ShaderLibrary or a HitGroup");

        if (m_hitGroupBuildContext)
        {
            m_hitGroupBuildContext->m_intersectionShaderName = intersectionShaderName;
        }
        else
        {
            m_shaderLibraryBuildContext->m_intersectionShaderName = intersectionShaderName;
        }

        return this;
    }

    DeviceRayTracingPipelineStateDescriptor* DeviceRayTracingPipelineStateDescriptor::HitGroup(const AZ::Name& hitGroupName)
    {
        ClearBuildContext();

        m_hitGroups.emplace_back();
        m_hitGroupBuildContext = &m_hitGroups.back();
        m_hitGroupBuildContext->m_hitGroupName = hitGroupName;
        return this;
    }

    void DeviceRayTracingPipelineStateDescriptor::ClearBuildContext()
    {
        m_shaderLibraryBuildContext = nullptr;
        m_hitGroupBuildContext = nullptr;
    }

    bool DeviceRayTracingPipelineStateDescriptor::IsTopLevelBuildContext()
    {
        return (m_shaderLibraryBuildContext == nullptr && m_hitGroupBuildContext == nullptr);
    }

    RHI::Ptr<RHI::DeviceRayTracingPipelineState> DeviceRayTracingPipelineState::CreateRHIRayTracingPipelineState()
    {
        RHI::Ptr<RHI::DeviceRayTracingPipelineState> rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
        AZ_Error("DeviceRayTracingPipelineState", rayTracingPipelineState, "Failed to create RHI::DeviceRayTracingPipelineState");
        return rayTracingPipelineState;
    }

    ResultCode DeviceRayTracingPipelineState::Init(Device& device, const DeviceRayTracingPipelineStateDescriptor* descriptor)
    {
        m_descriptor = *descriptor;

        ResultCode resultCode = InitInternal(device, descriptor);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }
        return resultCode;
    }

    void DeviceRayTracingPipelineState::Shutdown()
    {
        ShutdownInternal();
        DeviceObject::Shutdown();
    }
}
