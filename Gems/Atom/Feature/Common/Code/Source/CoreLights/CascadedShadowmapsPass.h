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
#include <AzCore/std/containers/fixed_vector.h>
#include <CoreLights/ShadowmapAtlas.h>
#include <CoreLights/ShadowmapPass.h>

namespace AZ
{
    namespace Render
    {
        //! CascadedShadowmapsPass owns and manages ShadowmapPasses.
        class CascadedShadowmapsPass final
            : public RPI::ParentPass
        {
            using Base = RPI::ParentPass;
            AZ_RPI_PASS(CascadedShadowmapsPass);

        public:
            AZ_RTTI(CascadedShadowmapsPass, "3956C19A-FBCB-4884-8AA9-3B47EFEC2B33", Base);
            AZ_CLASS_ALLOCATOR(CascadedShadowmapsPass, SystemAllocator, 0);

            virtual ~CascadedShadowmapsPass();
            static RPI::Ptr<CascadedShadowmapsPass> Create(const RPI::PassDescriptor& descriptor);

            //! This sets camera view name.
            void SetCameraViewName(const AZStd::string& viewName);

            //! This returns pipeline view tag for children.
            const AZStd::array_view<RPI::PipelineViewTag> GetPipelineViewTags();

            //! This exposes the shadowmap atlas.
            ShadowmapAtlas& GetShadowmapAtlas();

            //! This queues the image size and array size which will be updated in the beginning of the frame.
            void QueueForUpdateShadowmapImageSize(ShadowmapSize shadowmapSize, uint32_t arraySize);

        private:
            CascadedShadowmapsPass() = delete;
            explicit CascadedShadowmapsPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildInternal() override;
            void GetPipelineViewTags(RPI::SortedPipelineViewTags& outTags) const override;
            void GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, RPI::PassesByDrawList& outPassesByDrawList, const RPI::PipelineViewTag& viewTag) const override;

            RPI::Ptr<ShadowmapPass> CreateChild(uint16_t cascadeIndex);

            void UpdateChildren();

            void SetCascadesCount(uint16_t cascadesCount);
            void UpdateShadowmapImageSize();

            const Name m_slotName{ "Shadowmap" };
            Name m_drawListTagName;
            RHI::DrawListTag m_drawListTag;

            // The base name of the pipeline view tags for the children.
            RPI::PipelineViewTag m_basePipelineViewTag;

            // The name of the camera view associated to the shadow.
            // It is used to generate distinct child's pipeline view tags for each camera view.
            AZStd::string m_cameraViewName;

            // Generated pipeline view tags for the children (ShadowmapPass).
            AZStd::fixed_vector<RPI::PipelineViewTag, Shadow::MaxNumberOfCascades> m_childrenPipelineViewTags;
            uint16_t m_numberOfCascades = 0;

            ShadowmapAtlas m_atlas;

            ShadowmapSize m_shadowmapSize = ShadowmapSize::None;
            uint32_t m_arraySize = 1;
            bool m_updateChildren = true;
        };
    } // namespace Render
} // namespace AZ
