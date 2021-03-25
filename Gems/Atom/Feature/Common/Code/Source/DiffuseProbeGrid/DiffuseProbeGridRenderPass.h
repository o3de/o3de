/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass renders the diffuse global illumination in the area covered by
        //! each DiffuseProbeGrid.
        class DiffuseProbeGridRenderPass final
            : public RPI::RasterPass
        {
            using Base = RPI::RasterPass;
            AZ_RPI_PASS(DiffuseProbeGridRenderPass);
      
        public:
            AZ_RTTI(DiffuseProbeGridRenderPass, "{33F79A39-2DB3-46FC-8BA1-9E43E222C322}", Base);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRenderPass, SystemAllocator, 0);

            ~DiffuseProbeGridRenderPass() = default;
            static RPI::Ptr<DiffuseProbeGridRenderPass> Create(const RPI::PassDescriptor& descriptor);
        
        private:
            DiffuseProbeGridRenderPass() = delete;
            explicit DiffuseProbeGridRenderPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            virtual void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            Data::Instance<RPI::Shader> m_shader;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_srgAsset;
        };
    } // namespace Render
} // namespace AZ
