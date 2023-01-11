/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <CoreLights/ShadowmapAtlas.h>
#include <CoreLights/ShadowmapPass.h>


namespace AZ
{
    namespace Render
    {
        //! ProjectedShadowmapsPass owns shadowmap passes for projected lights.
        class ProjectedShadowmapsPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(ProjectedShadowmapsPass);
            using Base = RPI::ParentPass;

        public:
            AZ_CLASS_ALLOCATOR(ProjectedShadowmapsPass, SystemAllocator, 0);
            AZ_RTTI(ProjectedShadowmapsPass, "00024B13-1095-40FA-BEC3-B0F68110BEA2", Base);

            static constexpr uint16_t InvalidIndex = std::numeric_limits<uint16_t>::max();
            struct ShadowPassProperties
            {
                ShadowmapSize m_size = ShadowmapSize::None;
                uint16_t m_shadowIndexInSrg = InvalidIndex;
                bool m_isCached = false;
            };

            virtual ~ProjectedShadowmapsPass();
            static RPI::Ptr<ProjectedShadowmapsPass> Create(const RPI::PassDescriptor& descriptor);

            //! This returns true if this pass is of the given render pipeline. 
            bool IsOfRenderPipeline(const RPI::RenderPipeline& renderPipeline) const;

            //! This returns the pipeline view tag used in shadowmap passes.
            const RPI::PipelineViewTag& GetPipelineViewTagOfChild(size_t childIndex);

            //! This updates shadow map properties such as size, index, and if it is cached
            //! @param span of properties for all shadows.
            void UpdateShadowPassProperties(const AZStd::span<ShadowPassProperties>& shadowPassProperties);

            //! Forces the pass referenced by the given shadow index to render next frame. Useful if the shadow's view has moved.
            void ForceRenderNextFrame(uint16_t shadowIndex) const;

            //! This returns the image size(width/height) of shadowmap atlas.
            //! @return image size of the shadowmap atlas.
            ShadowmapSize GetShadowmapAtlasSize() const;

            //! This returns the origin of shadowmap in the atlas.
            //! @param lightIndex index of light in SRG.
            //! @return array slice and origin in the slice for the light.
            ShadowmapAtlas::Origin GetOriginInAtlas(uint16_t index) const;

            //! This exposes the shadowmap atlas.
            ShadowmapAtlas& GetShadowmapAtlas();

        private:
            ProjectedShadowmapsPass() = delete;
            explicit ProjectedShadowmapsPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildInternal() override;
            void GetPipelineViewTags(RPI::SortedPipelineViewTags& outTags) const override;
            void GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, RPI::PassesByDrawList& outPassesByDrawList, const RPI::PipelineViewTag& viewTag) const override;
            void FrameBeginInternal(FramePrepareParams params) override;

            RHI::Ptr<ShadowmapPass> CreateChild(size_t childIndex);

            void CreateClearShadowDrawPacket();
            void UpdateChildren();
            void SetChildrenCount(size_t count);
            
            const Name m_slotName{ "Shadowmap" };
            Name m_pipelineViewTagBase;
            Name m_drawListTagName;
            RHI::DrawListTag m_drawListTag;
            AZStd::vector<RPI::PipelineViewTag> m_childrenPipelineViewTags;
            AZStd::vector<ShadowPassProperties> m_shadowProperties;
            AZStd::unordered_map<uint16_t, ShadowmapPass*> m_shadowIndicesToPass;
            Data::Instance<AZ::RPI::Shader> m_clearShadowShader;
            RHI::ConstPtr<AZ::RHI::DrawPacket> m_clearShadowDrawPacket;
            RHI::Handle<uint32_t> m_casterMovedBit = RHI::Handle<uint32_t>(0);

            ShadowmapAtlas m_atlas;
            bool m_updateChildren = true;
        };
    } // namespace Render
} // namespace AZ
