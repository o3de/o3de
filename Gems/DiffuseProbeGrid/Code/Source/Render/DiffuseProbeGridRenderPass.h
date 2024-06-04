/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGrid;

        //! This pass renders the diffuse global illumination in the area covered by
        //! each DiffuseProbeGrid.
        class DiffuseProbeGridRenderPass final
            : public RPI::RasterPass
        {
            using Base = RPI::RasterPass;
            AZ_RPI_PASS(DiffuseProbeGridRenderPass);
      
        public:
            AZ_RTTI(DiffuseProbeGridRenderPass, "{33F79A39-2DB3-46FC-8BA1-9E43E222C322}", Base);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRenderPass, SystemAllocator);

            ~DiffuseProbeGridRenderPass() = default;
            static RPI::Ptr<DiffuseProbeGridRenderPass> Create(const RPI::PassDescriptor& descriptor);
        
        private:
            DiffuseProbeGridRenderPass() = delete;
            explicit DiffuseProbeGridRenderPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            bool IsEnabled() const override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            // helper function to determine if a DiffuseProbeGrid should be rendered based on its state
            bool ShouldRender(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid);

            Data::Instance<RPI::Shader> m_shader;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
        };
    } // namespace Render
} // namespace AZ
