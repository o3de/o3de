/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingPipelineState.h>

namespace AZ
{
    namespace RHI
    {
        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::Build()
        {
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxPayloadSize(uint32_t maxPayloadSize)
        {
            AZ_Assert(IsTopLevelBuildContext(), "MaxPayloadSize can only be added to the top level of the RayTracingPipelineState");
            m_configuration.m_maxPayloadSize = maxPayloadSize;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxAttributeSize(uint32_t maxAttributeSize)
        {
            AZ_Assert(IsTopLevelBuildContext(), "MaxAttributeSize can only be added to the top level of the RayTracingPipelineState");
            m_configuration.m_maxAttributeSize = maxAttributeSize;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxRecursionDepth(uint32_t maxRecursionDepth)
        {
            AZ_Assert(IsTopLevelBuildContext(), "MaxRecursionDepth can only be added to the top level of the RayTracingPipelineState");
            m_configuration.m_maxRecursionDepth = maxRecursionDepth;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::PipelineState(const RHI::PipelineState* pipelineState)
        {
            AZ_Assert(IsTopLevelBuildContext(), "PipelineState can only be added to the top level of the RayTracingPipelineState");
            m_pipelineState = pipelineState;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor)
        {
            ClearBuildContext();

            m_shaderLibraries.push_back({descriptor});
            m_shaderLibraryBuildContext = &m_shaderLibraries.back();
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::RayGenerationShaderName(const AZ::Name& name)
        {
            AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "RayGenerationShaderName can only be added to a ShaderLibrary");
            m_shaderLibraryBuildContext->m_rayGenerationShaderName = name;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MissShaderName(const AZ::Name& name)
        {
            AZ_Assert(m_shaderLibraryBuildContext && m_hitGroupBuildContext == nullptr, "MissShaderName can only be added to a ShaderLibrary");
            m_shaderLibraryBuildContext->m_missShaderName = name;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ClosestHitShaderName(const AZ::Name& closestHitShaderName)
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

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::AnyHitShaderName(const AZ::Name& anyHitShaderName)
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

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::HitGroup(const AZ::Name& hitGroupName)
        {
            ClearBuildContext();

            m_hitGroups.emplace_back();
            m_hitGroupBuildContext = &m_hitGroups.back();
            m_hitGroupBuildContext->m_hitGroupName = hitGroupName;
            return this;
        }

        void RayTracingPipelineStateDescriptor::ClearBuildContext()
        {
            m_shaderLibraryBuildContext = nullptr;
            m_hitGroupBuildContext = nullptr;
        }

        bool RayTracingPipelineStateDescriptor::IsTopLevelBuildContext()
        {
            return (m_shaderLibraryBuildContext == nullptr && m_hitGroupBuildContext == nullptr);
        }

        RHI::Ptr<RHI::RayTracingPipelineState> RayTracingPipelineState::CreateRHIRayTracingPipelineState()
        {
            RHI::Ptr<RHI::RayTracingPipelineState> rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
            AZ_Error("RayTracingPipelineState", rayTracingPipelineState, "Failed to create RHI::RayTracingPipelineState");
            return rayTracingPipelineState;
        }

        ResultCode RayTracingPipelineState::Init(Device& device, const RayTracingPipelineStateDescriptor* descriptor)
        {
            m_descriptor = *descriptor;

            ResultCode resultCode = InitInternal(device, descriptor);
            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
            }
            return resultCode;
        }

        void RayTracingPipelineState::Shutdown()
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }
}
