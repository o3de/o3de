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
            m_useSpecializationConstants = shaderAsset->UseSpecializationConstants(supervariantIndex);
            return true;
        }

        ShaderVariant::~ShaderVariant()
        {

        }

        void ShaderVariant::ConfigurePipelineState(
            RHI::PipelineStateDescriptor& descriptor,
            const ShaderVariantId& specialization) const
        {
            ConfigurePipelineState(descriptor, ShaderOptionGroup(m_shaderAsset->GetShaderOptionGroupLayout(), specialization));
        }

        void ShaderVariant::ConfigurePipelineState(
            RHI::PipelineStateDescriptor& descriptor,
            const ShaderOptionGroup& specialization) const
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
                descriptorForDraw.m_geometryFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Geometry);
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

            if (m_useSpecializationConstants)
            {
                // Configure specialization data for the shader
                AZ_Assert(
                    specialization.GetShaderOptionLayout() == m_shaderAsset->GetShaderOptionGroupLayout(),
                    "OptionGroup for specialization is different to the one in the ShaderAsset");
                descriptor.m_specializationData.clear();
                ShaderOptionGroup options = specialization;
                options.SetUnspecifiedToDefaultValues();
                for (auto& option : options.GetShaderOptionLayout()->GetShaderOptions())
                {
                    if (option.GetSpecializationId() >= 0)
                    {
                        descriptor.m_specializationData.emplace_back();
                        auto& specializationData = descriptor.m_specializationData.back();
                        specializationData.m_name = option.GetName();
                        specializationData.m_id = option.GetSpecializationId();
                        specializationData.m_value = RHI::SpecializationValue(option.Get(options).GetIndex());
                        switch (option.GetType())
                        {
                        case ShaderOptionType::Boolean:
                            specializationData.m_type = RHI::SpecializationType::Bool;
                            break;
                        case ShaderOptionType::Enumeration:
                        case ShaderOptionType::IntegerRange:
                            specializationData.m_type = RHI::SpecializationType::Integer;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        void ShaderVariant::ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor) const
        {
            auto layout = m_shaderAsset->GetShaderOptionGroupLayout();
            for ([[maybe_unused]] auto& option : layout->GetShaderOptions())
            {
                if (m_useSpecializationConstants && option.GetSpecializationId() >= 0)
                {
                    AZ_Error(
                        "ConfigurePipelineState",
                        !m_useSpecializationConstants || option.GetSpecializationId() < 0,
                        "Configuring PipelineStateDescriptor without specializing option %s.\
                         Call ConfigurePipelineState with specialization data. Default value will be used.",
                        option.GetName().GetCStr());
                }
            }
            ConfigurePipelineState(descriptor, ShaderOptionGroup(layout));
        }

        bool ShaderVariant::IsFullySpecialized() const
        {
            return m_shaderAsset->IsFullySpecialized(m_supervariantIndex);
        }

        bool ShaderVariant::UseSpecializationConstants() const
        {
            return m_shaderAsset->UseSpecializationConstants(m_supervariantIndex);
        }

        bool ShaderVariant::UseKeyFallback() const
        {
            return !(IsFullyBaked() || IsFullySpecialized());
        }

    } // namespace RPI
} // namespace AZ
