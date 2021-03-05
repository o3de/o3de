/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_Assert(m_hitGroupBuildContext == nullptr && m_rootSignatureBuildContext == nullptr, "MaxPayloadSize can only be added to the top level of the RayTracingPipelineState");
            m_configuration.m_maxPayloadSize = maxPayloadSize;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxAttributeSize(uint32_t maxAttributeSize)
        {
            AZ_Assert(m_hitGroupBuildContext == nullptr && m_rootSignatureBuildContext == nullptr, "MaxAttributeSize can only be added to the top level of the RayTracingPipelineState");
            m_configuration.m_maxAttributeSize = maxAttributeSize;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::MaxRecursionDepth(uint32_t maxRecursionDepth)
        {
            AZ_Assert(m_hitGroupBuildContext == nullptr && m_rootSignatureBuildContext == nullptr, "MaxRecursionDepth can only be added to the top level of the RayTracingPipelineState");
            m_configuration.m_maxRecursionDepth = maxRecursionDepth;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor)
        {
            ClearBuildContext();

            m_shaderLibraries.push_back({ descriptor });
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

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ClosestHitShaderName(const AZ::Name& closestHitShaderName)
        {
            AZ_Assert(m_hitGroupBuildContext, "ClosestHitShaderName can only be added to a HitGroup");
            m_hitGroupBuildContext->m_closestHitShaderName = closestHitShaderName;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::AnyHitShaderName(const AZ::Name& anyHitShaderName)
        {
            AZ_Assert(m_hitGroupBuildContext, "AnyHitShaderName can only be added to a HitGroup");
            m_hitGroupBuildContext->m_anyHitShaderName = anyHitShaderName;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::GlobalRootSignature()
        {
            ClearBuildContext();

            m_rootSignatureBuildContext = &m_globalRootSignature;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::LocalRootSignature()
        {
            ClearBuildContext();

            m_localRootSignatures.emplace_back();
            m_localRootSignatureBuildContext = &m_localRootSignatures.back();
            m_rootSignatureBuildContext = m_localRootSignatureBuildContext;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::CbvParameter()
        {
            ClearParamBuildContext();

            AZ_Assert(m_rootSignatureBuildContext, "CbvParameter can only be added to a RootSignature");
            AZ_Assert(m_rootSignatureBuildContext->m_cbvParam.m_addedToRootSignature == false, "Only one CbvParameter can be added to a RootSignature");
            m_rootSignatureCbvParamBuildContext = &m_rootSignatureBuildContext->m_cbvParam;
            m_rootSignatureBuildContext->m_cbvParam.m_addedToRootSignature = true;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::DescriptorTableParameter()
        {
            ClearParamBuildContext();

            AZ_Assert(m_rootSignatureBuildContext, "DescriptorTableParameter can only be added to a RootSignature");
            AZ_Assert(m_rootSignatureBuildContext->m_descriptorTableParam.m_addedToRootSignature == false, "Only one DescriptorTableParameter can be added to a RootSignature");
            m_rootSignatureDescriptorTableParamBuildContext = &m_rootSignatureBuildContext->m_descriptorTableParam;
            m_rootSignatureBuildContext->m_descriptorTableParam.m_addedToRootSignature = true;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::RootParameterIndex(uint32_t rootParameterIndex)
        {
            AZ_Assert(m_rootSignatureCbvParamBuildContext
                || m_rootSignatureDescriptorTableParamBuildContext
                || m_rootSignatureStaticSamplerBuildContext, "RootParameterIndex can only be added to a RootSignature parameter or static sampler");

            if (m_rootSignatureCbvParamBuildContext)
            {
                m_rootSignatureCbvParamBuildContext->m_rootParameterIndex = rootParameterIndex;
            }
            else if (m_rootSignatureDescriptorTableParamBuildContext)
            {
                m_rootSignatureDescriptorTableParamBuildContext->m_rootParameterIndex = rootParameterIndex;
            }
            else
            {
                m_rootSignatureStaticSamplerBuildContext->m_rootParameterIndex = rootParameterIndex;
            }
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ShaderRegister(uint32_t shaderRegister)
        {
            AZ_Assert(m_rootSignatureCbvParamBuildContext
                || m_rootSignatureDescriptorTableParamRangeBuildContext
                || m_rootSignatureStaticSamplerBuildContext, "ShaderRegister can only be added to a RootSignature parameter or static sampler");

            if (m_rootSignatureCbvParamBuildContext)
            {
                m_rootSignatureCbvParamBuildContext->m_shaderRegister = shaderRegister;
            }
            else if (m_rootSignatureDescriptorTableParamRangeBuildContext)
            {
                m_rootSignatureDescriptorTableParamRangeBuildContext->m_shaderRegister = shaderRegister;
            }
            else
            {
                m_rootSignatureStaticSamplerBuildContext->m_shaderRegister = shaderRegister;
            }
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::RegisterSpace(uint32_t registerSpace)
        {
            AZ_Assert(m_rootSignatureCbvParamBuildContext
                || m_rootSignatureDescriptorTableParamRangeBuildContext
                || m_rootSignatureStaticSamplerBuildContext, "RegisterSpace can only be added to a RootSignature parameter or static sampler");

            if (m_rootSignatureCbvParamBuildContext)
            {
                m_rootSignatureCbvParamBuildContext->m_registerSpace = registerSpace;
            }
            else if (m_rootSignatureDescriptorTableParamRangeBuildContext)
            {
                m_rootSignatureDescriptorTableParamRangeBuildContext->m_registerSpace = registerSpace;
            }
            else
            {
                m_rootSignatureStaticSamplerBuildContext->m_registerSpace = registerSpace;
            }
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::Range()
        {
            AZ_Assert(m_rootSignatureDescriptorTableParamBuildContext, "Range can only be added to a RootSignature DescriptorTable parameter");
            m_rootSignatureDescriptorTableParamBuildContext->m_ranges.emplace_back();
            m_rootSignatureDescriptorTableParamRangeBuildContext = &m_rootSignatureDescriptorTableParamBuildContext->m_ranges.back();
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::RangeType(RayTracingRootSignatureDescriptorTableParam::Range::Type rangeType)
        {
            AZ_Assert(m_rootSignatureDescriptorTableParamRangeBuildContext, "RangeType can only be added to a RootSignature DescriptorTable Range parameter");
            m_rootSignatureDescriptorTableParamRangeBuildContext->m_type = rangeType;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::ShaderAssociation(const AZ::Name& shaderName)
        {
            AZ_Assert(m_localRootSignatureBuildContext, "ShaderAssociation can only be added to a LocalRootSignature");
            m_localRootSignatureBuildContext->m_shaderAssociations.push_back(shaderName);
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::StaticSampler()
        {
            ClearParamBuildContext();

            m_rootSignatureBuildContext->m_staticSamplers.emplace_back();
            m_rootSignatureStaticSamplerBuildContext = &m_rootSignatureBuildContext->m_staticSamplers.back();
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::FilterMode(RHI::FilterMode filterMode)
        {
            AZ_Assert(m_rootSignatureStaticSamplerBuildContext, "FilterMode can only be added to a StaticSampler parameter");
            m_rootSignatureStaticSamplerBuildContext->m_filterMode = filterMode;
            return this;
        }

        RayTracingPipelineStateDescriptor* RayTracingPipelineStateDescriptor::AddressMode(RHI::AddressMode addressMode)
        {
            AZ_Assert(m_rootSignatureStaticSamplerBuildContext, "AddressMode can only be added to a StaticSampler parameter");
            m_rootSignatureStaticSamplerBuildContext->m_addressMode = addressMode;
            return this;
        }

        void RayTracingPipelineStateDescriptor::ClearBuildContext()
        {
            m_hitGroupBuildContext = nullptr;
            m_rootSignatureBuildContext = nullptr;
            m_localRootSignatureBuildContext = nullptr;
            m_rootSignatureStaticSamplerBuildContext = nullptr;

            ClearParamBuildContext();
        }

        void RayTracingPipelineStateDescriptor::ClearParamBuildContext()
        {
            m_rootSignatureCbvParamBuildContext = nullptr;
            m_rootSignatureDescriptorTableParamBuildContext = nullptr;
            m_rootSignatureDescriptorTableParamRangeBuildContext = nullptr;
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
        }
    }
}
