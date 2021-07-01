/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/DepthExponentiationPass.h>

#include <Atom/Feature/CoreLights/EsmShadowmapsPassData.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Math/MathFilterDescriptor.h>
#include <Math/MathFilter.h>

namespace AZ
{
    namespace Render
    {
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
        }

        const Name& EsmShadowmapsPass::GetLightTypeName() const
        {
            return m_lightTypeName;
        }

        void EsmShadowmapsPass::SetFilterParameters(const AZStd::array_view<float>& standardDeviations)
        {
            // Set descriptor for Gaussian filters for given set of standard deviations.
            MathFilterDescriptor descriptor;
            descriptor.m_kind = MathFilterKind::Gaussian;
            descriptor.m_gaussians.reserve(standardDeviations.size());
            for (const float standardDeviation : standardDeviations)
            {
                descriptor.m_gaussians.emplace_back(GaussianFilterDescriptor{ standardDeviation });
            }

            // Set filter paramter buffer along with element counts for each filter.
            MathFilter::BufferWithElementCounts bufferCounts = MathFilter::FindOrCreateFilterBuffer(descriptor);
            m_filterTableBuffer = bufferCounts.first;
            m_filterCounts = AZStd::move(bufferCounts.second);
        }

        AZStd::array_view<uint32_t> EsmShadowmapsPass::GetFilterCounts() const
        {
            return m_filterCounts;
        }

        void EsmShadowmapsPass::SetShadowmapIndexTableBuffer(const Data::Instance<RPI::Buffer>& tableBuffer)
        {
            m_shadowmapIndexTableBuffer = tableBuffer;
        }

        void EsmShadowmapsPass::SetFilterParameterBuffer(const Data::Instance<RPI::Buffer>& dataBuffer)
        {
            m_filterParameterBuffer = dataBuffer;
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
                if (m_lightTypeName == Name("directional"))
                {
                    exponentiationPass->SetShadowmapType(Shadow::ShadowmapType::Directional);
                }
                else if (m_lightTypeName == Name("projected"))
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

        void EsmShadowmapsPass::ResetInternal()
        {
            SetEnabledComputation(m_computationEnabled);
            Base::ResetInternal();
        }

        void EsmShadowmapsPass::FrameBeginInternal(FramePrepareParams params)
        {
            UpdateChildren();

            Base::FrameBeginInternal(params);
        }

        void EsmShadowmapsPass::UpdateChildren()
        {
            const RPI::PassAttachmentBinding& inputBinding = GetInputBinding(0);
            AZ_Assert(inputBinding.m_attachment->m_descriptor.m_type == RHI::AttachmentType::Image, "[EsmShadowmapsPass %s] input attachment requires an image attachment", GetPathName().GetCStr());
            m_shadowmapImageSize = inputBinding.m_attachment->m_descriptor.m_image.m_size;
            m_shadowmapArraySize = inputBinding.m_attachment->m_descriptor.m_image.m_arraySize;

            const AZStd::array_view<RPI::Ptr<RPI::Pass>>& children = GetChildren();
            AZ_Assert(children.size() == EsmChildPassKindCount, "[EsmShadowmapsPass '%s'] The count of children is wrong.", GetPathName().GetCStr());

            for (uint32_t index = 0; index < EsmChildPassKindCount; ++index)
            {
                RPI::ComputePass* child = azrtti_cast<RPI::ComputePass*>(children[index].get());
                AZ_Assert(child, "[EsmShadowmapsPass '%s'] A child does not compute.", GetPathName().GetCStr());

                Data::Instance<RPI::ShaderResourceGroup> srg = child->GetShaderResourceGroup();

                if (m_shadowmapIndexTableBufferIndices[index].IsNull())
                {
                    m_shadowmapIndexTableBufferIndices[index] = srg->FindShaderInputBufferIndex(Name("m_shadowmapIndexTable"));
                }
                srg->SetBuffer(m_shadowmapIndexTableBufferIndices[index], m_shadowmapIndexTableBuffer);

                if (m_filterParameterBufferIndices[index].IsNull())
                {
                    m_filterParameterBufferIndices[index] = srg->FindShaderInputBufferIndex(Name("m_filterParameters"));
                }
                srg->SetBuffer(m_filterParameterBufferIndices[index], m_filterParameterBuffer);

                if (index != static_cast<uint32_t>(EsmChildPassKind::Exponentiation))
                {
                    if (m_filterTableBufferIndices[index].IsNull())
                    {
                        m_filterTableBufferIndices[index] = srg->FindShaderInputBufferIndex(Name("m_filterTable"));
                    }
                    srg->SetBuffer(m_filterTableBufferIndices[index], m_filterTableBuffer);
                }

                child->SetTargetThreadCounts(
                    m_shadowmapImageSize.m_width,
                    m_shadowmapImageSize.m_height,
                    m_shadowmapArraySize);
            }
        }

    } // namespace Render
} // namespace AZ
