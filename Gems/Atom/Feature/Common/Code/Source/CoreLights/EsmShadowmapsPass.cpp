/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/DepthExponentiationPass.h>

#include <Atom/Feature/CoreLights/EsmShadowmapsPassData.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Math/MathFilterDescriptor.h>
#include <Math/MathFilter.h>

namespace AZ
{
    namespace Render
    {
        // --- Pass Creation ---

        RPI::Ptr<EsmShadowmapsPass> EsmShadowmapsPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew EsmShadowmapsPass(descriptor);
        }

        EsmShadowmapsPass::EsmShadowmapsPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            const EsmShadowmapsPassData* esmData = RPI::PassUtils::GetPassData<EsmShadowmapsPassData>(descriptor);

            if (!esmData)
            {
                return;
            }

            m_lightTypeName = esmData->m_lightType;

            if (m_lightTypeName == Name("projected"))
            {
                m_lightType = EsmLightType::Projected;
            }
            else if (m_lightTypeName == Name("directional"))
            {
                m_lightType = EsmLightType::Directional;
            }
        }

        // --- Setters/Getters ---

        const Name& EsmShadowmapsPass::GetLightTypeName() const
        {
            return m_lightTypeName;
        }

        bool EsmShadowmapsPass::GetIsProjected() const
        {
            return m_lightType == EsmLightType::Projected;
        }

        void EsmShadowmapsPass::SetShadowmapIndexTableBuffer(const Data::Instance<RPI::Buffer>& tableBuffer)
        {
            m_shadowmapIndexTableBuffer = tableBuffer;
        }

        void EsmShadowmapsPass::SetFilterParameterBuffer(const Data::Instance<RPI::Buffer>& dataBuffer)
        {
            m_filterParameterBuffer = dataBuffer;
        }

        void EsmShadowmapsPass::SetEsmExponent(float exponent)
        {
            m_esmExponent = exponent;
        }

        void EsmShadowmapsPass::SetBlurParameters(Data::Instance<RPI::ShaderResourceGroup> srg, const uint32_t childPassIndex)
        {
            if (m_shadowmapIndexTableBufferIndices[childPassIndex].IsNull())
            {
                m_shadowmapIndexTableBufferIndices[childPassIndex] = srg->FindShaderInputBufferIndex(Name("m_shadowmapIndexTable"));
            }
            srg->SetBuffer(m_shadowmapIndexTableBufferIndices[childPassIndex], m_shadowmapIndexTableBuffer);

            if (m_filterParameterBufferIndices[childPassIndex].IsNull())
            {
                m_filterParameterBufferIndices[childPassIndex] = srg->FindShaderInputBufferIndex(Name("m_filterParameters"));
            }
            srg->SetBuffer(m_filterParameterBufferIndices[childPassIndex], m_filterParameterBuffer);

            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_esmExponent")), m_esmExponent);
        }

        void EsmShadowmapsPass::SetKawaseBlurSpecificParameters(Data::Instance<RPI::ShaderResourceGroup> srg, uint32_t kawaseBlurIndex)
        {
            if (m_kawaseBlurConstantIndices[kawaseBlurIndex].IsNull())
            {
                m_kawaseBlurConstantIndices[kawaseBlurIndex] = srg->FindShaderInputConstantIndex(Name("m_rcpResolutionAndIteration"));
            }
            const Vector4 data(
                1.0f / m_shadowmapImageSize.m_width, 1.0f / m_shadowmapImageSize.m_height, aznumeric_cast<float>(kawaseBlurIndex), 0.0f);

            srg->SetConstant(m_kawaseBlurConstantIndices[kawaseBlurIndex], data);
        }

        // --- Frame Render ---

        void EsmShadowmapsPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetEnabledComputation(m_computationEnabled);
            if (m_computationEnabled)
            {
                UpdateChildren();
            }
            Base::FrameBeginInternal(params);
        }

        void EsmShadowmapsPass::BuildInternal()
        {
            if (GetInputOutputCount() > 0 && GetIsProjected())
            {
                if (!m_atlasAttachmentImage)
                {
                    auto binding = GetInputOutputBinding(0);
                    if (binding.GetAttachment() == nullptr)
                    {
                        // Make sure at least a dummy image is attached if lacking any other attachments.
                        // This may be necessary for a few frames during initialization.
                        auto tempAttachmentImage = RPI::ImageSystemInterface::Get()->GetSystemAttachmentImage(RHI::Format::R16_FLOAT);
                        AttachImageToSlot(Name("EsmShadowmaps"), tempAttachmentImage);
                    }
                }
                else
                {
                    AttachImageToSlot(Name("EsmShadowmaps"), m_atlasAttachmentImage);
                }
            }

            Base::BuildInternal();
        }

        void EsmShadowmapsPass::SetEnabledComputation(bool enabled)
        {
            if (m_lightTypeName.IsEmpty())
            {
                return;
            }

            for (const RPI::Ptr<RPI::Pass>& child : GetChildren())
            {
                child->SetEnabled(enabled);
            }

            if (enabled)
            {
                auto* exponentiationPass = azrtti_cast<DepthExponentiationPass*>(GetChildren()[static_cast<uint32_t>(EsmChildPassKind::Exponentiation)].get());
                AZ_Assert(exponentiationPass, "Child not found or not of type DepthExponentiationPass.");
                if (m_lightType == EsmLightType::Directional)
                {
                    exponentiationPass->SetShadowmapType(Shadow::ShadowmapType::Directional);
                }
                else if (m_lightType == EsmLightType::Projected)
                {
                    exponentiationPass->SetShadowmapType(Shadow::ShadowmapType::Projected);
                }
                else
                {
                    AZ_Assert(false, "LightType is illegal");
                }
            }

            m_computationEnabled = enabled;
        }

        void EsmShadowmapsPass::SetAtlasAttachmentImage(Data::Instance<RPI::AttachmentImage> atlasAttachmentIamge)
        {
            if (m_atlasAttachmentImage != atlasAttachmentIamge)
            {
                m_atlasAttachmentImage = atlasAttachmentIamge;
                QueueForBuildAndInitialization();
            }
        }

        void EsmShadowmapsPass::UpdateChildren()
        {
            const RPI::PassAttachmentBinding& inputBinding = GetInputBinding(0);

            if (!inputBinding.GetAttachment())
            {
                AZ_Assert(false, "[EsmShadowmapsPass %s] requires an input attachment", GetPathName().GetCStr());
                return;
            }

            AZ_Assert(inputBinding.GetAttachment()->m_descriptor.m_type == RHI::AttachmentType::Image, "[EsmShadowmapsPass %s] input attachment requires an image attachment", GetPathName().GetCStr());
            m_shadowmapImageSize = inputBinding.GetAttachment()->m_descriptor.m_image.m_size;
            m_shadowmapArraySize = inputBinding.GetAttachment()->m_descriptor.m_image.m_arraySize;

            const AZStd::span<const RPI::Ptr<RPI::Pass>>& children = GetChildren();
            AZ_Assert(children.size() == EsmChildPassKindCount, "[EsmShadowmapsPass '%s'] The count of children is wrong.", GetPathName().GetCStr());

            for (uint32_t childPassIndex = 0; childPassIndex < EsmChildPassKindCount; ++childPassIndex)
            {
                RPI::ComputePass* child = azrtti_cast<RPI::ComputePass*>(children[childPassIndex].get());
                AZ_Assert(child, "[EsmShadowmapsPass '%s'] A child does not compute.", GetPathName().GetCStr());

                Data::Instance<RPI::ShaderResourceGroup> srg = child->GetShaderResourceGroup();

                SetBlurParameters(srg, childPassIndex);
                if (childPassIndex >= aznumeric_cast<uint32_t>(EsmChildPassKind::KawaseBlur0))
                {
                    SetKawaseBlurSpecificParameters(srg, childPassIndex - aznumeric_cast<uint32_t>(EsmChildPassKind::KawaseBlur0));
                }

                child->SetTargetThreadCounts(
                    m_shadowmapImageSize.m_width,
                    m_shadowmapImageSize.m_height,
                    m_shadowmapArraySize);
            }
        }
    } // namespace Render
} // namespace AZ
