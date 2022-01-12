/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/ShaderVariant.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>

namespace AZ
{
    namespace RPI
    {
        bool ShaderVariant::Init(
            const Data::Asset<ShaderAsset>& shaderAsset,
            const Data::Asset<ShaderVariantAsset>& shaderVariantAsset,
            SupervariantIndex supervariantIndex)
        {
            m_shaderAsset = shaderAsset;
            m_shaderVariantAsset = shaderVariantAsset;
            m_supervariantIndex = supervariantIndex;
            m_pipelineStateType = shaderAsset->GetPipelineStateType();
            m_pipelineLayoutDescriptor = shaderAsset->GetPipelineLayoutDescriptor(supervariantIndex);
            m_renderStates = &shaderAsset->GetRenderStates(supervariantIndex);

            return true;
        }

        ShaderVariant::~ShaderVariant()
        {

        }

        void ShaderVariant::ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor) const
        {
            descriptor.m_pipelineLayoutDescriptor = m_pipelineLayoutDescriptor;

            switch (descriptor.GetType())
            {
            case RHI::PipelineStateType::Draw:
            {
                AZ_Assert(m_pipelineStateType == RHI::PipelineStateType::Draw, "ShaderVariant is not intended for the raster pipeline.");
                AZ_Assert(m_renderStates, "Invalid RenderStates");
                RHI::PipelineStateDescriptorForDraw& descriptorForDraw = static_cast<RHI::PipelineStateDescriptorForDraw&>(descriptor);
                descriptorForDraw.m_vertexFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Vertex);
                descriptorForDraw.m_tessellationFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Tessellation);
                descriptorForDraw.m_fragmentFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Fragment);
                descriptorForDraw.m_renderStates = *m_renderStates;
                break;
            }

            case RHI::PipelineStateType::Dispatch:
            {
                AZ_Assert(m_pipelineStateType == RHI::PipelineStateType::Dispatch, "ShaderVariant is not intended for the compute pipeline.");
                RHI::PipelineStateDescriptorForDispatch& descriptorForDispatch = static_cast<RHI::PipelineStateDescriptorForDispatch&>(descriptor);
                descriptorForDispatch.m_computeFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Compute);
                break;
            }

            case RHI::PipelineStateType::RayTracing:
            {
                AZ_Assert(m_pipelineStateType == RHI::PipelineStateType::RayTracing, "ShaderVariant is not intended for the ray tracing pipeline.");
                RHI::PipelineStateDescriptorForRayTracing& descriptorForRayTracing = static_cast<RHI::PipelineStateDescriptorForRayTracing&>(descriptor);
                descriptorForRayTracing.m_rayTracingFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::RayTracing);
                break;
            }

            default:
                AZ_Assert(false, "Unexpected PipelineStateType");
                break;
            }
        }

    } // namespace RPI
} // namespace AZ
