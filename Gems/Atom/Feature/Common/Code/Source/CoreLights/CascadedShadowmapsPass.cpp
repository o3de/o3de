/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        CascadedShadowmapsPass::CascadedShadowmapsPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(descriptor);
            if (passData)
            {
                m_drawListTagName = passData->m_drawListTag;
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(passData->m_drawListTag);
                m_basePipelineViewTag = passData->m_pipelineViewTag;
            }

            QueueForUpdateShadowmapImageSize(ShadowmapSize::None, 1);
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

        void CascadedShadowmapsPass::SetCameraViewName(const AZStd::string& viewName)
        {
            if (m_cameraViewName != viewName)
            {
                m_cameraViewName = viewName;
                m_childrenPipelineViewTags.clear();
                RemoveChildren();

                if (m_numberOfCascades)
                {
                    // No need to remove children as if the cascaded number changed it will
                    // be updated by SetCascadesCount that will remove and adjust the children.
                    // We keep it in place to be on the safe side as this is a new camera view
                    // and we rather start fresh.
                    GetPipelineViewTags();
                    SetCascadesCount(m_numberOfCascades);
                }
                else
                {   // This should not really happen - the minimum is 1
                    AZ_RPI_PASS_WARNING(false,
                        "CascadedShadowmapsPass::SetCameraViewName - cascaded shadows amount should be greater than 0");
                } 
            }
        }

        const AZStd::array_view<RPI::PipelineViewTag> CascadedShadowmapsPass::GetPipelineViewTags()
        {
            if (m_childrenPipelineViewTags.size() != Shadow::MaxNumberOfCascades)
            {
                m_childrenPipelineViewTags.resize(Shadow::MaxNumberOfCascades);
                for (uint16_t cascadeIndex = 0; cascadeIndex < Shadow::MaxNumberOfCascades; ++cascadeIndex)
                {
                    // These pipeline view tags are used to distinguish transient views,
                    // so we offer distinct tag for each cascade index and for each camera view.
                    m_childrenPipelineViewTags[cascadeIndex] =
                        AZStd::string::format("%s_%d_%s",
                            m_basePipelineViewTag.GetCStr(),
                            cascadeIndex,
                            m_cameraViewName.c_str());
                }
            }
            return m_childrenPipelineViewTags;
        }

        void CascadedShadowmapsPass::QueueForUpdateShadowmapImageSize(ShadowmapSize shadowmapSize, uint32_t arraySize)
        {
            m_shadowmapSize = shadowmapSize;
            m_arraySize = arraySize;
            m_updateChildren = true;

            QueueForBuildAndInitialization();

            m_atlas.Initialize();
            for (size_t cascadeIndex = 0; cascadeIndex < m_arraySize; ++cascadeIndex)
            {
                m_atlas.SetShadowmapSize(cascadeIndex, m_shadowmapSize);
            }
            m_atlas.Finalize();
        }

        void CascadedShadowmapsPass::UpdateChildren()
        {
            if (!m_updateChildren)
            {
                return;
            }
            m_updateChildren = false;

            if (m_atlas.GetBaseShadowmapSize() == ShadowmapSize::None)
            {
                // Even when no shadow is given, an execution of child
                // is required to transit the shadowmap image resource.
                SetCascadesCount(1);
                auto* pass = static_cast<ShadowmapPass*>(GetChildren()[0].get());
                pass->SetArraySlice(0);
                const RHI::Size imageSize{ 1, 1, 1 };
                pass->SetViewportScissorFromImageSize(imageSize);
                return;
            }

            SetCascadesCount(m_arraySize);
            const RHI::Size imageSize
            {
                aznumeric_cast<uint32_t>(m_shadowmapSize),
                aznumeric_cast<uint32_t>(m_shadowmapSize),
                m_arraySize
            };
            for (RPI::Ptr<RPI::Pass> child : GetChildren())
            {
                auto* shadowmapPass = static_cast<ShadowmapPass*>(child.get());
                shadowmapPass->SetViewportScissorFromImageSize(imageSize);
            }
        }

        ShadowmapAtlas& CascadedShadowmapsPass::GetShadowmapAtlas()
        {
            return m_atlas;
        }

        void CascadedShadowmapsPass::BuildInternal()
        {
            UpdateChildren();

            if (GetChildren().empty())
            {
                SetCascadesCount(1);
            }

            UpdateShadowmapImageSize();
            Base::BuildInternal();
        }

        void CascadedShadowmapsPass::GetPipelineViewTags(RPI::SortedPipelineViewTags& outTags) const
        {
            for (size_t childIndex = 0; childIndex < m_numberOfCascades; ++childIndex)
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

        RPI::Ptr<ShadowmapPass> CascadedShadowmapsPass::CreateChild(uint16_t cascadeIndex)
        {
            const AZStd::array_view<RPI::PipelineViewTag> childrenViewTags = GetPipelineViewTags();
            const Name passName{ AZStd::string::format("DirectionalLightShadowmapPass.%d", cascadeIndex) };

            auto passData = AZStd::make_shared<RPI::RasterPassData>();
            passData->m_drawListTag = m_drawListTagName;
            passData->m_pipelineViewTag = childrenViewTags[cascadeIndex];            

            auto pass = ShadowmapPass::CreateWithPassRequest(passName, passData);
            pass->SetArraySlice(cascadeIndex);
            return pass;
        }

        void CascadedShadowmapsPass::SetCascadesCount(uint16_t cascadesCount)
        {
            AZ_Assert(cascadesCount > 0, "The number of cascades must be positive.");

            // Orphans unnecessary children.
            while (GetChildren().size() > cascadesCount)
            {
                RemoveChild(GetChildren()[cascadesCount]);
            }

            // Creates new children.
            const uint16_t oldCascadeCount = aznumeric_cast<uint16_t>(GetChildren().size());
            for (uint16_t cascadeIndex = oldCascadeCount; cascadeIndex < cascadesCount; ++cascadeIndex)
            {
                RPI::Ptr<ShadowmapPass> child = CreateChild(cascadeIndex);
                AZ_RPI_PASS_WARNING(child, "CascadedShadowmapsPass child Pass creation failed for %d", cascadeIndex);
                if (child)
                {
                    child->QueueForBuildAndInitialization();
                    AddChild(child);
                }
            }
            m_numberOfCascades = cascadesCount;
        }

        void CascadedShadowmapsPass::UpdateShadowmapImageSize()
        {
            // [GFX TODO][ATOM-2470] stop caring about attachment
            RPI::Ptr<RPI::PassAttachment> attachment = m_ownedAttachments.front();
            if (!attachment)
            {
                AZ_Assert(false, "[CascadedShadowmapsPass %s] Cannot find shadowmap image attachment.", GetPathName().GetCStr());
                return;
            }
            AZ_Assert(attachment->m_descriptor.m_type == RHI::AttachmentType::Image, "[CascadedShadowmapsPass %s] requires an image attachment", GetPathName().GetCStr());

            RPI::PassAttachmentBinding& binding = GetOutputBinding(0);
            binding.m_attachment = attachment;

            RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
            const uint32_t shadowmapWidth = static_cast<uint32_t>(m_atlas.GetBaseShadowmapSize());
            imageDescriptor.m_size = RHI::Size(shadowmapWidth, shadowmapWidth, 1);
            imageDescriptor.m_arraySize = m_atlas.GetArraySliceCount();
        }

    } // namespace Render
} // namespace AZ
