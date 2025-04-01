/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

namespace AZ
{
    namespace Render
    {
        //! ShadowmapPass outputs shadowmap depth images.
        class ShadowmapPass final
            : public RPI::RasterPass
        {
            friend class CascadedShadowmapsPass;
            friend class ProjectedShadowmapsPass;

            using Base = RPI::RasterPass;
            AZ_RPI_PASS(ShadowmapPass);
      
        public:
            AZ_RTTI(ShadowmapPass, "FCBDDB8C-E565-4780-9E2E-B45F16203F77", Base);
            AZ_CLASS_ALLOCATOR(ShadowmapPass, SystemAllocator);

            ~ShadowmapPass() = default;

            static RPI::Ptr<ShadowmapPass> Create(const RPI::PassDescriptor& descriptor);

            // Creates a common pass template for the child shadowmap passes.
            static void CreatePassTemplate();

            //! Creates a pass descriptor from the input, using the ShadowmapPassTemplate, and adds a pass request to connect to the parent pass.
            //! This function assumes the parent pass has a SkinnedMeshes input slot
            static RPI::Ptr<ShadowmapPass> CreateWithPassRequest(const Name& passName, AZStd::shared_ptr<RPI::RasterPassData> passData);

            //! This updates array slice for this shadowmap.
            void SetArraySlice(uint16_t arraySlice);

            //! This enable/disable clearing of the image view.
            void SetClearEnabled(bool enabled);

            //! Sets this shadow pass to only update its shadows when a mesh in view moves.
            void SetIsStatic(bool isStatic);

            //! Sets which bit to check to see if a caster in the view moved.
            void SetCasterMovedBit(RHI::Handle<uint32_t> bit);

            //! When the shadow is static, this forces the shadow to still re-render next frame (due to the light moving for instance)
            void ForceRenderNextFrame();

            //! This update viewport and scissor for this shadowmap from the given image size.
            void SetViewportScissorFromImageSize(const RHI::Size& imageSize);

            //! This updates viewport and scissor for this shadowmap.
            void SetViewportScissor(const RHI::Viewport& viewport, const RHI::Scissor& scissor);

            //! Sets the draw packet used for clearing a shadow viewport.
            void SetClearShadowDrawPacket(AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> clearShadowDrawPacket);

        private:
            ShadowmapPass() = delete;
            explicit ShadowmapPass(const RPI::PassDescriptor& descriptor);

            // Sets the underlying pipeline view tag. Purpose of this function is to allow parent shadow passes to update the view tag.
            void UpdatePipelineViewTag(const RPI::PipelineViewTag& viewTag);

            // RHI::Pass overrides...
            void BuildInternal() override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void FrameEndInternal() override;

            // RPI::RasterPass overrides...
            void SubmitDrawItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t indexOffset) const override;

            // Gets the number of expected draws, taking into account if this shadow is static.
            uint32_t GetNumDraws() const;

            RHI::ConstPtr<RHI::DrawPacket> m_clearShadowDrawPacket;
            RHI::DrawItemProperties m_clearShadowDrawItemProperties;
            RHI::Handle<uint32_t> m_casterMovedBit;
            uint16_t m_arraySlice = 0;
            bool m_clearEnabled = true;
            bool m_isStatic = false;
            uint32_t m_lastFrameDrawCount = 0;
            mutable bool m_forceRenderNextFrame = false;
        };
    } // namespace Render
} // namespace AZ
