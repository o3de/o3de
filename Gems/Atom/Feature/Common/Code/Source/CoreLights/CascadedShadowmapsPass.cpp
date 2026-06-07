/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <CoreLights/CascadedShadowmapsPass.h>
#include <CoreLights/ShadowmapPass.h>

namespace AZ
{
    namespace Render
    {
        // Pass Creation ...

        CascadedShadowmapsPass::CascadedShadowmapsPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(descriptor);
            if (passData)
            {
                m_drawListTagName = passData->m_drawListTag;
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(passData->m_drawListTag);
            }

            SetShadowmapSize(ShadowmapSize::None, 1);
        }

        CascadedShadowmapsPass::~CascadedShadowmapsPass()
        {
            if (m_drawListTag.IsValid())
            {
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                rhiSystem->GetDrawListTagRegistry()->ReleaseTag(m_drawListTag);
            }
        }

        RPI::Ptr<CascadedShadowmapsPass> CascadedShadowmapsPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew CascadedShadowmapsPass(descriptor);
        }

        // Child pass creation ...

        void CascadedShadowmapsPass::CreateChildShadowMapPass(u16 cascadeIndex)
        {
            const Name passName{ AZStd::string::format("%d", cascadeIndex) };
            auto passData = AZStd::make_shared<RPI::RasterPassData>();
            passData->m_drawListTag = m_drawListTagName;
            passData->m_pipelineViewTag = GetPipelineViewTags()[cascadeIndex];
            passData->m_bindViewSrg = true;

            auto pass = ShadowmapPass::CreateWithPassRequest(passName, passData);

            const RHI::Size imageSize
            {
                aznumeric_cast<uint32_t>(m_shadowmapSize),
                aznumeric_cast<uint32_t>(m_shadowmapSize),
                m_numCascades
            };
            pass->SetViewportScissorFromImageSize(imageSize);
            pass->SetArraySlice(cascadeIndex);

            AddChild(pass);
        }

        void CascadedShadowmapsPass::CreateChildPassesInternal()
        {
            for (u16 childIdx = 0; childIdx < m_numCascades; ++childIdx)
            {
                CreateChildShadowMapPass(childIdx);
            }
        }

        void CascadedShadowmapsPass::BuildInternal()
        {
            UpdateShadowmapImageSize();
            Base::BuildInternal();
        }

        void CascadedShadowmapsPass::SetShadowmapSize(ShadowmapSize shadowmapSize, u16 numCascades)
        {
            AZ_Assert(numCascades > 0, "The number of cascades must be positive.");

            bool rebuildPasses = (numCascades != m_numCascades) || (shadowmapSize != m_shadowmapSize);
            m_numCascades = numCascades;
            m_shadowmapSize = shadowmapSize;

            if (rebuildPasses)
            {
                m_flags.m_createChildren = true;
                QueueForBuildAndInitialization();
            }

            m_atlas.Initialize();
            for (size_t cascadeIndex = 0; cascadeIndex < m_numCascades; ++cascadeIndex)
            {
                m_atlas.SetShadowmapSize(cascadeIndex, m_shadowmapSize);
            }
            m_atlas.Finalize();
        }

        void CascadedShadowmapsPass::UpdateShadowmapImageSize()
        {
            // [GFX TODO][ATOM-2470] stop caring about attachment
            RPI::Ptr<RPI::PassAttachment> attachment = m_ownedAttachments.front();
            AZ_Assert(attachment, "[CascadedShadowmapsPass %s] Cannot find shadowmap image attachment.", GetPathName().GetCStr());
            AZ_Assert(attachment->m_descriptor.m_type == RHI::AttachmentType::Image, "[CascadedShadowmapsPass %s] requires an image attachment", GetPathName().GetCStr());

            RPI::PassAttachmentBinding& binding = GetOutputBinding(0);
            binding.SetAttachment(attachment);

            RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
            const uint32_t shadowmapWidth = static_cast<uint32_t>(m_atlas.GetBaseShadowmapSize());
            imageDescriptor.m_size = RHI::Size(shadowmapWidth, shadowmapWidth, 1);
            imageDescriptor.m_arraySize = m_atlas.GetArraySliceCount();
        }

        // View related ...

        void CascadedShadowmapsPass::SetCameraViewName(const AZStd::string& viewName)
        {
            if (m_cameraViewName != viewName)
            {
                m_cameraViewName = viewName;
                m_childrenPipelineViewTags.clear();
                GetPipelineViewTags();
                for (size_t i = 0; i < m_children.size(); ++i)
                {
                    ShadowmapPass* shadowPass = azrtti_cast<ShadowmapPass*>(m_children[i].get());
                    shadowPass->UpdatePipelineViewTag(GetPipelineViewTags()[i]);
                }
            }
        }

        const AZStd::span<const RPI::PipelineViewTag> CascadedShadowmapsPass::GetPipelineViewTags()
        {
            if (m_childrenPipelineViewTags.size() != Shadow::MaxNumberOfCascades)
            {
                auto basePipelineViewTag{ GetPipelineViewTag() };
                m_childrenPipelineViewTags.resize(Shadow::MaxNumberOfCascades);
                for (uint16_t cascadeIndex = 0; cascadeIndex < Shadow::MaxNumberOfCascades; ++cascadeIndex)
                {
                    // These pipeline view tags are used to distinguish transient views, so we offer distinct tag for each cascade index and for each camera view.
                    m_childrenPipelineViewTags[cascadeIndex] =
                        AZStd::string::format("%s_%d_%s", basePipelineViewTag.GetCStr(), cascadeIndex, m_cameraViewName.c_str());
                }
            }
            return m_childrenPipelineViewTags;
        }

        void CascadedShadowmapsPass::GetPipelineViewTags(RPI::PipelineViewTags& outTags) const
        {
            for (size_t childIndex = 0; childIndex < m_numCascades; ++childIndex)
            {
                outTags.insert(m_childrenPipelineViewTags[childIndex]);
            }
        }

        void CascadedShadowmapsPass::GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, RPI::PassesByDrawList& outPassesByDrawList, const RPI::PipelineViewTag& viewTag) const
        {
            if (AZStd::find(
                m_childrenPipelineViewTags.begin(),
                m_childrenPipelineViewTags.end(),
                viewTag) != m_childrenPipelineViewTags.end() &&
                outPassesByDrawList.find(m_drawListTag) == outPassesByDrawList.end())
            {
                outPassesByDrawList[m_drawListTag] = this;
                outDrawListMask.set(m_drawListTag.GetIndex());
            }
        }

    } // namespace Render
} // namespace AZ
