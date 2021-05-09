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

#include <Atom/RHI.Reflect/Size.h>

#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace RHI
    {
        struct DrawItem;
    }

    namespace Render
    {
        namespace Hair
        {
            class HairRenderObject;
            class HairFeatureProcessor;

            static const char* const HairPPLLRasterPassTemplateName = "HairPPLLRasterPassTemplate";

            //! A HairPPLLRasterPass is used for the hair render after the data was processed
            //!  through the skinning and simulation passes.
            //! The output of this pass is an unordered pixels data list that can now be
            //!  traversed in order to resolve depth and compute lighting.
            //! The Fill pass is using the following Srgs:
            //!  - PerPassSrg shared by all hair passes for the shared dynamic buffer and the PPLL buffers
            //!  - PerMaterialSrg - used solely by this pass for the visual hair properties.
            //!  - HairDynamicDataSrg (PerObjectSrg) - shared buffers views for this hair object only.
            //!  - PerViewSrg - for now this will be filled per pass, however in the near future it
            //!     should be replaced by the Atom PerViewSrg and calculated only once per view.
            class HairPPLLRasterPass
                : public RPI::RasterPass
                , private RPI::ShaderReloadNotificationBus::Handler
            {
                AZ_RPI_PASS(HairPPLLRasterPass);

            public:
                AZ_RTTI(HairPPLLRasterPass, "{6614D7DD-24EE-4A2B-B314-7C035E2FB3C4}", RasterPass);
                AZ_CLASS_ALLOCATOR(HairPPLLRasterPass, SystemAllocator, 0);
                virtual ~HairPPLLRasterPass();

                //! Creates a HairPPLLRasterPass
                static RPI::Ptr<HairPPLLRasterPass> Create(const RPI::PassDescriptor& descriptor);

                bool BuildDrawPacket(HairRenderObject* hairObject);
                bool AddDrawPacket(HairRenderObject* hairObject);

                Data::Instance<RPI::Shader> GetShader() { return m_shader; }

                void SetFeatureProcessor(HairFeatureProcessor* featureProcessor)
                {
                    m_featureProcessor = featureProcessor;
                }

                virtual bool IsEnabled() const override;
            protected:
                // ShaderReloadNotificationBus::Handler overrides...
                void OnShaderReinitialized(const RPI::Shader& shader) override;
                void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
                void OnShaderVariantReinitialized(const RPI::Shader& shader, const RPI::ShaderVariantId& shaderVariantId, RPI::ShaderVariantStableId shaderVariantStableId) override;

            private:
                explicit HairPPLLRasterPass(const RPI::PassDescriptor& descriptor);

                bool LoadShaderAndPipelineState();

                // Pass behavior overrides
                void BuildAttachmentsInternal() override;
                void OnBuildAttachmentsFinishedInternal() override;
                void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;

                // Scope producer functions...
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            private:
                HairFeatureProcessor* m_featureProcessor = nullptr;

                RHI::Size  m_attachmentSize = RHI::Size(1,1,0);    // for buffer set.

                // The  shader that will be used by the pass
                Data::Instance<RPI::Shader> m_shader = nullptr;


                // To help create the pipeline state 
                RPI::PassDescriptor m_passDescriptor;

                const RHI::PipelineState* m_pipelineState = nullptr;

                AZStd::mutex m_mutex;

                //! list of dispatch items, each represents a single hair object that
                //!  will be used by the skinning compute shader.
                AZStd::unordered_set<const RHI::DrawPacket*> m_drawPackets;

                bool m_initialized = false;
            };

        } // namespace Hair
    } // namespace Render
}   // namespace AZ
