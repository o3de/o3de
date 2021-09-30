/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            //! A HairPPLLRasterPass is used for the hair fragments fill render after the data
            //!  went through the skinning and simulation passes.
            //! The output of this pass is the general list of fragment data that can now be
            //!  traversed for depth resolve and lighting.
            //! The Fill pass uses the following Srgs:
            //!  - PerPassSrg shared by all hair passes for the shared dynamic buffer and the PPLL buffers
            //!  - PerMaterialSrg - used solely by this pass to alter the vertices and apply the visual
            //!     hair properties to each fragment.
            //!  - HairDynamicDataSrg (PerObjectSrg) - shared buffers views for this hair object only.
            //!  - PerViewSrg and PerSceneSrg - as per the data from Atom.
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

                bool AddDrawPackets(AZStd::list<Data::Instance<HairRenderObject>>& hairObjects);

                //! The following will be called when an object was added or shader has been compiled
                void SchedulePacketBuild(HairRenderObject* hairObject);

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
                void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            private:
                explicit HairPPLLRasterPass(const RPI::PassDescriptor& descriptor);

                bool LoadShaderAndPipelineState();
                bool AcquireFeatureProcessor();
                void BuildShaderAndRenderData();
                bool BuildDrawPacket(HairRenderObject* hairObject);

                // Pass behavior overrides
                void InitializeInternal() override;
                void BuildInternal() override;
                void FrameBeginInternal(FramePrepareParams params) override;

                // Scope producer functions...
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            private:
                HairFeatureProcessor* m_featureProcessor = nullptr;

                // The  shader that will be used by the pass
                Data::Instance<RPI::Shader> m_shader = nullptr;

                // To help create the pipeline state 
                RPI::PassDescriptor m_passDescriptor;

                const RHI::PipelineState* m_pipelineState = nullptr;
                RPI::ViewPtr m_currentView = nullptr;

                AZStd::mutex m_mutex;

                //! List of new render objects introduced this frame so that their
                //!  Per Object (dynamic) Srg should be bound to the resources.
                //! Done once per every new object introduced / requires update.
                AZStd::unordered_set<HairRenderObject*> m_newRenderObjects;

                bool m_initialized = false;
            };

        } // namespace Hair
    } // namespace Render
}   // namespace AZ
