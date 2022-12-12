/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <CoreLights/ProjectedShadowmapsPass.h>
#include <AzCore/std/iterator.h>
#include <Atom/Feature/Mesh/MeshCommon.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ProjectedShadowmapsPass> ProjectedShadowmapsPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew ProjectedShadowmapsPass(descriptor);
        }

        ProjectedShadowmapsPass::ProjectedShadowmapsPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            // Pass has it's own logic for managing children, forgo ParentPass logic
            m_flags.m_createChildren = false;

            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(descriptor);
            if (passData)
            {
                m_drawListTagName = passData->m_drawListTag;
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(passData->m_drawListTag);
                m_pipelineViewTagBase = passData->m_pipelineViewTag;
            }

            AZStd::vector<ShadowPassProperties> shadowPassProperties;
            shadowPassProperties.push_back(ShadowPassProperties{ ShadowmapSize::None, 0, false });
            UpdateShadowPassProperties(shadowPassProperties);
        }

        ProjectedShadowmapsPass::~ProjectedShadowmapsPass()
        {
            if (m_drawListTag.IsValid())
            {
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                rhiSystem->GetDrawListTagRegistry()->ReleaseTag(m_drawListTag);
            }
        }

        bool ProjectedShadowmapsPass::IsOfRenderPipeline(const RPI::RenderPipeline& renderPipeline) const
        {
            return &renderPipeline == m_pipeline;         
        }

        const RPI::PipelineViewTag& ProjectedShadowmapsPass::GetPipelineViewTagOfChild(size_t childIndex)
        {
            m_childrenPipelineViewTags.reserve(childIndex + 1);
            while (m_childrenPipelineViewTags.size() <= childIndex)
            {
                const size_t tagIndex = m_childrenPipelineViewTags.size();
                m_childrenPipelineViewTags.emplace_back(
                    AZStd::string::format("%s.%zu", m_pipelineViewTagBase.GetCStr(), tagIndex));
            }
            return m_childrenPipelineViewTags[childIndex];
        }

        void ProjectedShadowmapsPass::UpdateShadowPassProperties(const AZStd::span<ShadowPassProperties>& properties)
        {
            m_shadowProperties.assign(properties.begin(), properties.end());
            m_updateChildren = true;
            QueueForBuildAndInitialization();

            m_atlas.Initialize();
            for (const auto& it : m_shadowProperties)
            {
                m_atlas.SetShadowmapSize(it.m_shadowIndexInSrg, it.m_size);
            }
            m_atlas.Finalize();
        }

        void ProjectedShadowmapsPass::ForceRenderNextFrame(uint16_t shadowIndex) const
        {
            const auto& it = m_shadowIndicesToPass.find(shadowIndex);
            if (it != m_shadowIndicesToPass.end())
            {
                it->second->ForceRenderNextFrame();
            }
        }

        void ProjectedShadowmapsPass::UpdateChildren()
        {
            if (!m_updateChildren)
            {
                return;
            }
            m_updateChildren = false;
            m_shadowIndicesToPass.clear();

            if (m_atlas.GetBaseShadowmapSize() == ShadowmapSize::None)
            {
                // Even when no shadow is given, an execution of child
                // is required to transit the shadowmap image resource.
                SetChildrenCount(1);
                auto* pass = static_cast<ShadowmapPass*>(GetChildren()[0].get());
                pass->SetArraySlice(0);
                const RHI::Viewport viewport{ 0.f, 1.f, 0.f, 1.f };
                const RHI::Scissor scissor{ 0, 0, 1, 1 };
                pass->SetViewportScissor(viewport, scissor);
                return;
            }

            const size_t shadowmapCount = m_shadowProperties.size();
            SetChildrenCount(shadowmapCount);

            struct SliceInfo
            {
                bool m_hasStaticShadows = false;
                AZStd::vector<ShadowmapPass*> m_shadowPasses;
            };

            AZStd::vector<SliceInfo> sliceInfo(m_atlas.GetArraySliceCount());
            for (const auto& it : m_shadowProperties)
            {
                // This index indicates the execution order of the passes.
                // The first pass to render a slice should clear the slice.
                const ptrdiff_t index = AZStd::distance(m_shadowProperties.cbegin(), &it);
                auto* pass = static_cast<ShadowmapPass*>(GetChildren()[index].get());
                m_shadowIndicesToPass[it.m_shadowIndexInSrg] = pass;

                const ShadowmapAtlas::Origin origin = m_atlas.GetOrigin(it.m_shadowIndexInSrg);
                pass->SetArraySlice(origin.m_arraySlice);
                pass->SetIsStatic(it.m_isCached);
                pass->ForceRenderNextFrame();

                if (it.m_size != ShadowmapSize::None)
                {
                    const uint32_t sizeInInt = static_cast<uint32_t>(it.m_size);
                    const RHI::Viewport viewport(
                        origin.m_originInSlice[0] * 1.f,
                        (origin.m_originInSlice[0] + sizeInInt) * 1.f,
                        origin.m_originInSlice[1] * 1.f,
                        (origin.m_originInSlice[1] + sizeInInt) * 1.f);
                    const RHI::Scissor scissor(
                        origin.m_originInSlice[0],
                        origin.m_originInSlice[1],
                        origin.m_originInSlice[0] + sizeInInt,
                        origin.m_originInSlice[1] + sizeInInt);
                    pass->SetViewportScissor(viewport, scissor);
                    pass->SetClearEnabled(false);

                    SliceInfo& sliceInfoItem = sliceInfo.at(origin.m_arraySlice);
                    sliceInfoItem.m_shadowPasses.push_back(pass);
                    sliceInfoItem.m_hasStaticShadows = sliceInfoItem.m_hasStaticShadows || it.m_isCached;
                }
            }

            for (const auto& it : sliceInfo)
            {
                if (!it.m_hasStaticShadows)
                {
                    if (!it.m_shadowPasses.empty())
                    {
                        // no static shadows in this slice, so have the first pass clear the atlas on load.
                        it.m_shadowPasses.at(0)->SetClearEnabled(true);
                    }
                }
                else
                {
                    // There's at least one static shadow in this slice, so passes need to clear themselves using a draw.
                    for (auto* pass : it.m_shadowPasses)
                    {
                        pass->SetClearShadowDrawPacket(m_clearShadowDrawPacket);
                        pass->SetCasterMovedBit(m_casterMovedBit);
                    }
                }
            }
        }

        ShadowmapSize ProjectedShadowmapsPass::GetShadowmapAtlasSize() const
        {
            return m_atlas.GetBaseShadowmapSize();
        }

        ShadowmapAtlas::Origin ProjectedShadowmapsPass::GetOriginInAtlas(uint16_t index) const
        {
            return m_atlas.GetOrigin(index);
        }

        ShadowmapAtlas& ProjectedShadowmapsPass::GetShadowmapAtlas()
        {
            return m_atlas;
        }

        void ProjectedShadowmapsPass::BuildInternal()
        {
            UpdateChildren();

            // [GFX TODO][ATOM-2470] stop caring about attachment
            RPI::Ptr<RPI::PassAttachment> attachment = m_ownedAttachments.front();
            if (!attachment)
            {
                AZ_Assert(false, "[ProjectedShadowmapsPass %s] Cannot find shadowmap image attachment.", GetPathName().GetCStr());
                return;
            }
            AZ_Assert(attachment->m_descriptor.m_type == RHI::AttachmentType::Image, "[ProjectedShadowmapsPass %s] requires an image attachment", GetPathName().GetCStr());

            RPI::PassAttachmentBinding& binding = GetOutputBinding(0);
            binding.SetAttachment(attachment);

            RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
            const uint32_t shadowmapWidth = static_cast<uint32_t>(m_atlas.GetBaseShadowmapSize());
            imageDescriptor.m_size = RHI::Size(shadowmapWidth, shadowmapWidth, 1);
            imageDescriptor.m_arraySize = m_atlas.GetArraySliceCount();

            Base::BuildInternal();
        }

        void ProjectedShadowmapsPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_clearShadowDrawPacket)
            {
                CreateClearShadowDrawPacket();
                m_updateChildren = true;
            }
            m_casterMovedBit = GetScene()->GetViewTagBitRegistry().FindTag(MeshCommon::MeshMovedName);
            Base::FrameBeginInternal(params);
        }

        void ProjectedShadowmapsPass::GetPipelineViewTags(RPI::SortedPipelineViewTags& outTags) const
        {
            const size_t childrenCount = GetChildren().size();
            AZ_Assert(m_childrenPipelineViewTags.size() >= childrenCount, "There are not enough pipeline view tags.");
            for (size_t childIndex = 0; childIndex < childrenCount; ++childIndex)
            {
                outTags.insert(m_childrenPipelineViewTags[childIndex]);
            }
        }

        void ProjectedShadowmapsPass::GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, RPI::PassesByDrawList& outPassesByDrawList, const RPI::PipelineViewTag& viewTag) const
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

        RPI::Ptr<ShadowmapPass> ProjectedShadowmapsPass::CreateChild(size_t childIndex)
        {
            const Name passName{ AZStd::string::format("ProjectedShadowmapPass.%zu", childIndex) };

            auto passData = AZStd::make_shared<RPI::RasterPassData>();
            passData->m_drawListTag = m_drawListTagName;
            passData->m_pipelineViewTag = GetPipelineViewTagOfChild(childIndex);

            return ShadowmapPass::CreateWithPassRequest(passName, passData);
        }

        void ProjectedShadowmapsPass::CreateClearShadowDrawPacket()
        {
            // Force load of shader to clear shadow maps.
            const AZStd::string clearShadowShaderFilePath = "Shaders/Shadow/ClearShadow.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>
                (clearShadowShaderFilePath, RPI::AssetUtils::TraceLevel::Assert);

            m_clearShadowShader = AZ::RPI::Shader::FindOrCreate(shaderAsset);
            const AZ::RPI::ShaderVariant& variant = m_clearShadowShader->GetRootVariant();

            AZ::RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            variant.ConfigurePipelineState(pipelineStateDescriptor);

            GetScene()->ConfigurePipelineState(m_clearShadowShader->GetDrawListTag(), pipelineStateDescriptor);

            AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
            pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

            const AZ::RHI::PipelineState* pipelineState = m_clearShadowShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!pipelineState)
            {
                AZ_Assert(false, "Shader '%s'. Failed to acquire default pipeline state", shaderAsset->GetName().GetCStr());
                return;
            }

            AZ::RHI::DrawPacketBuilder drawPacketBuilder;
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetDrawArguments(AZ::RHI::DrawLinear(1, 0, 3, 0));

            AZ::RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = m_clearShadowShader->GetDrawListTag();
            drawRequest.m_pipelineState = pipelineState;
            drawRequest.m_sortKey = AZStd::numeric_limits<RHI::DrawItemSortKey>::min();

            drawPacketBuilder.AddDrawItem(drawRequest);
            m_clearShadowDrawPacket = drawPacketBuilder.End();
        }

        void ProjectedShadowmapsPass::SetChildrenCount(size_t childrenCount)
        {
            // Reserve Tags
            if (childrenCount > 0)
            {
                GetPipelineViewTagOfChild(childrenCount - 1);
            }

            // Orphans unnecessary children.
            while (GetChildren().size() > childrenCount)
            {
                RemoveChild(GetChildren()[childrenCount]);
            }

            // Creates new children.
            const size_t lastChildrenCount = GetChildren().size();
            for (size_t childIndex = lastChildrenCount; childIndex < childrenCount; ++childIndex)
            {
                RPI::Ptr<ShadowmapPass> child = CreateChild(childIndex);
                AddChild(child);
            }
        }


    } // namespace Render
} // namespace AZ
