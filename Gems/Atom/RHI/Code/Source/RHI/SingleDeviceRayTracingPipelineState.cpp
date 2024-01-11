/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/SingleDeviceRayTracingPipelineState.h>

namespace AZ::RHI
{
    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::Build()
    {
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::MaxPayloadSize(uint32_t maxPayloadSize)
    {
        AZ_Assert(IsTopLevelBuildContext(), "MaxPayloadSize can only be added to the top level of the SingleDeviceRayTracingPipelineState");
        m_configuration.m_maxPayloadSize = maxPayloadSize;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::MaxAttributeSize(uint32_t maxAttributeSize)
    {
        AZ_Assert(IsTopLevelBuildContext(), "MaxAttributeSize can only be added to the top level of the SingleDeviceRayTracingPipelineState");
        m_configuration.m_maxAttributeSize = maxAttributeSize;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::MaxRecursionDepth(uint32_t maxRecursionDepth)
    {
        AZ_Assert(IsTopLevelBuildContext(), "MaxRecursionDepth can only be added to the top level of the SingleDeviceRayTracingPipelineState");
        m_configuration.m_maxRecursionDepth = maxRecursionDepth;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::PipelineState(const RHI::SingleDevicePipelineState* pipelineState)
    {
        AZ_Assert(IsTopLevelBuildContext(), "SingleDevicePipelineState can only be added to the top level of the SingleDeviceRayTracingPipelineState");
        m_pipelineState = pipelineState;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor)
    {
        ClearBuildContext();

        m_shaderLibraries.push_back({descriptor});
        m_shaderLibraryBuildContext = &m_shaderLibraries.back();
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::RayGenerationShaderName(const AZ::Name& name)
    {
        AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "RayGenerationShaderName can only be added to a ShaderLibrary");
        m_shaderLibraryBuildContext->m_rayGenerationShaderName = name;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::MissShaderName(const AZ::Name& name)
    {
        AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "MissShaderName can only be added to a ShaderLibrary");
        m_shaderLibraryBuildContext->m_missShaderName = name;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::CallableShaderName(const AZ::Name& callableShaderName)
    {
        AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "CallableShaderName can only be added to a ShaderLibrary");
        m_shaderLibraryBuildContext->m_callableShaderName = callableShaderName;
        return this;
    }

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::ClosestHitShaderName(const AZ::Name& closestHitShaderName)
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

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::AnyHitShaderName(const AZ::Name& anyHitShaderName)
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

    SingleDeviceRayTracingPipelineStateDescriptor* SingleDeviceRayTracingPipelineStateDescriptor::HitGroup(const AZ::Name& hitGroupName)
    {
        ClearBuildContext();

        m_hitGroups.emplace_back();
        m_hitGroupBuildContext = &m_hitGroups.back();
        m_hitGroupBuildContext->m_hitGroupName = hitGroupName;
        return this;
    }

    void SingleDeviceRayTracingPipelineStateDescriptor::ClearBuildContext()
    {
        m_shaderLibraryBuildContext = nullptr;
        m_hitGroupBuildContext = nullptr;
    }

    bool SingleDeviceRayTracingPipelineStateDescriptor::IsTopLevelBuildContext()
    {
        return (m_shaderLibraryBuildContext == nullptr && m_hitGroupBuildContext == nullptr);
    }

    RHI::Ptr<RHI::SingleDeviceRayTracingPipelineState> SingleDeviceRayTracingPipelineState::CreateRHIRayTracingPipelineState()
    {
        RHI::Ptr<RHI::SingleDeviceRayTracingPipelineState> rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
        AZ_Error("SingleDeviceRayTracingPipelineState", rayTracingPipelineState, "Failed to create RHI::SingleDeviceRayTracingPipelineState");
        return rayTracingPipelineState;
    }

    ResultCode SingleDeviceRayTracingPipelineState::Init(Device& device, const SingleDeviceRayTracingPipelineStateDescriptor* descriptor)
    {
        m_descriptor = *descriptor;

        ResultCode resultCode = InitInternal(device, descriptor);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }
        return resultCode;
    }

    void SingleDeviceRayTracingPipelineState::Shutdown()
    {
        ShutdownInternal();
        DeviceObject::Shutdown();
    }
}
