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

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that takes the results of the LightCulling shader and bins the results.
        //! This allows the forward shader to quickly look up all the lights that affect it by isolating the exact bin with the indices
        //! then walk through them in linear order
        //! Also we are converting from R32 to R16 so it is more read efficient
        class LightCullingRemap final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(LightCullingRemap);

        public:
            AZ_RTTI(AZ::Render::LightCullingRemap, "{0ED3D558-F7B3-4D3F-B9E6-3A3B8EC30E91}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(LightCullingRemap, SystemAllocator, 0);
            virtual ~LightCullingRemap() = default;

            /// Creates a LightCullingRemap
            static RPI::Ptr<LightCullingRemap> Create(const RPI::PassDescriptor& descriptor);

            static Name GetLightCullingRemapTemplateName()
            {
                return Name("LightCullingRemapTemplate");
            }

        private:

            LightCullingRemap(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void ResetInternal()override;
            void BuildInternal() override;

            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            void CreateRemappedLightListBuffer();
            void Init();
            AZ::RHI::Size GetTileDataBufferResolution();
            uint32_t FindInputOutputBinding(const AZ::Name& name);

            AZ::RHI::ShaderInputConstantIndex m_tileWidthIndex;
            Data::Instance<RPI::Buffer> m_lightListRemapped;
            AZ::RHI::Size m_tileDim;
            int m_tileDataIndex = -1;
            bool m_initialized = false;
        };
    }   // namespace Render
}   // namespace AZ
