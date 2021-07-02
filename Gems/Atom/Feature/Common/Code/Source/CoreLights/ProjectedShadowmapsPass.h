/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <AtomCore/std/containers/array_view.h>
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

            static constexpr uint16_t InvalidIndex = ~0;
            struct ShadowmapSizeWithIndices
            {
                ShadowmapSize m_size = ShadowmapSize::None;
                uint16_t m_shadowIndexInSrg = InvalidIndex;
            };

            virtual ~ProjectedShadowmapsPass();
            static RPI::Ptr<ProjectedShadowmapsPass> Create(const RPI::PassDescriptor& descriptor);

            //! This returns true if this pass is of the given render pipeline. 
            bool IsOfRenderPipeline(const RPI::RenderPipeline& renderPipeline) const;

            //! This returns the pipeline view tag used in shadowmap passes.
            const RPI::PipelineViewTag& GetPipelineViewTagOfChild(size_t childIndex);

            //! This update shadowmap sizes for each projected light shadow index.
            //! @param sizes shadowmap sizes for each projected light shadow index.
            void UpdateShadowmapSizes(const AZStd::vector<ShadowmapSizeWithIndices>& sizes);

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

            RHI::Ptr<ShadowmapPass> CreateChild(size_t childIndex);

            void UpdateChildren();
            void SetChildrenCount(size_t count);
            
            const Name m_slotName{ "Shadowmap" };
            Name m_pipelineViewTagBase;
            Name m_drawListTagName;
            RHI::DrawListTag m_drawListTag;
            AZStd::vector<RPI::PipelineViewTag> m_childrenPipelineViewTags;
            AZStd::vector<ShadowmapSizeWithIndices> m_sizes;

            ShadowmapAtlas m_atlas;
            bool m_updateChildren = true;
        };
    } // namespace Render
} // namespace AZ
