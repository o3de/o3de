/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

// Hair specific
#include <TressFX/TressFXConstantBuffers.h>

#include <Passes/HairSkinningComputePass.h>

#include <Passes/HairPPLLRasterPass.h>
#include <Passes/HairPPLLResolvePass.h>

#include <Passes/HairShortCutGeometryDepthAlphaPass.h>
#include <Passes/HairShortCutGeometryShadingPass.h>

#include <Rendering/HairRenderObject.h>
#include <Rendering/SharedBuffer.h>
#include <Rendering/HairCommon.h>
#include <Rendering/HairGlobalSettingsBus.h>

namespace AZ
{
    namespace RHI
    {
        struct ImageDescriptor;
    }

    namespace Render
    {
        namespace Hair
        {
            //! The following lines dictate the overall memory consumption reserved for the
            //!  PPLL fragments.  The memory consumption using this technique is quite large (can
            //!  grow far above 1GB in GPU/CPU data and in extreme cases of zoom up with dense hair
            //!  might still not be enough.  For this reason it is recommended to utilize the
            //!  approximated lighting scheme originally suggested by Eidos Montreal and do OIT using
            //!  several frame buffer layers for storing closest fragments data.
            //! Using the approximated technique, the OIT buffers will consume roughly 256MB for 4K
            //!  resolution render with 4 OIT layers.
            static const size_t PPLL_NODE_SIZE = 16;
            static const size_t AVE_FRAGS_PER_PIXEL = 24;
            static const size_t SCREEN_WIDTH = 1920;
            static const size_t SCREEN_HEIGHT = 1080;
            static const size_t RESERVED_PIXELS_FOR_OIT = SCREEN_WIDTH * SCREEN_HEIGHT * AVE_FRAGS_PER_PIXEL;

            class HairSkinningPass;
            class HairRenderObject;

            //! The HairFeatureProcessor (FP) is the glue between the various hair components / entities in
            //!  the scene and their passes / shaders.
            //! The FP will keep track of all active hair objects, will run their skinning update iteration
            //!  and will then populate them into each of the passes to be computed and rendered.
            //! The overall process involves update, skinning, collision, and simulation compute, fragment
            //!  raster fill, and final frame buffer OIT resolve.
            //! The last part can be switched to support the smaller foot print pass version that instead
            //!  of fragments list (PPLL) will use fill screen buffers to approximate OIT layer resolve.
            class HairFeatureProcessor final
                : public RPI::FeatureProcessor
                , public HairGlobalSettingsRequestBus::Handler
                , private AZ::TickBus::Handler
            {
                Name HairParentPassName;

                // Compute passes
                Name GlobalShapeConstraintsPassName;
                Name CalculateStrandDataPassName;
                Name VelocityShockPropagationPassName;
                Name LocalShapeConstraintsPassName;
                Name LengthConstriantsWindAndCollisionPassName;
                Name UpdateFollowHairPassName;

                // PPLL render passes
                Name HairPPLLRasterPassName;
                Name HairPPLLResolvePassName;

                // ShortCut render passes
                Name HairShortCutGeometryDepthAlphaPassName;
                Name HairShortCutResolveDepthPassName;
                Name HairShortCutGeometryShadingPassName;
                Name HairShortCutResolveColorPassName;

            public:
                AZ_RTTI(AZ::Render::Hair::HairFeatureProcessor, "{5F9DDA81-B43F-4E30-9E56-C7C3DC517A4C}", RPI::FeatureProcessor);

                static void Reflect(AZ::ReflectContext* context);

                HairFeatureProcessor();
                virtual ~HairFeatureProcessor();


                void UpdateHairSkinning();

                bool Init(RPI::RenderPipeline* pipeline);
                bool IsInitialized() { return m_initialized; }

                // FeatureProcessor overrides ...
                void Activate() override;
                void Deactivate() override;
                void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
                void Render(const FeatureProcessor::RenderPacket& packet) override;

                // AZ::TickBus::Handler overrides
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
                int GetTickOrder() override;

                void AddHairRenderObject(Data::Instance<HairRenderObject> renderObject);
                bool RemoveHairRenderObject(Data::Instance<HairRenderObject> renderObject);

                // RPI::SceneNotificationBus overrides ...
                void OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline) override;
                void OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline) override;
                void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

                Data::Instance<HairSkinningComputePass> GetHairSkinningComputegPass();
                Data::Instance<HairPPLLRasterPass> GetHairPPLLRasterPass();
                Data::Instance<RPI::Shader> GetGeometryRasterShader();

                //! Update the hair objects materials array.
                void FillHairMaterialsArray(std::vector<const AMD::TressFXRenderParams*>& renderSettings);

                Data::Instance<RPI::Buffer> GetPerPixelListBuffer() { return m_linkedListNodesBuffer; }
                HairUniformBuffer<AMD::TressFXShadeParams>& GetMaterialsArray() { return m_hairObjectsMaterialsCB;  }

                void ForceRebuildRenderData() { m_forceRebuildRenderData = true; }
                void SetAddDispatchEnable(bool enable) { m_addDispatchEnabled = enable; }
                void SetEnable(bool enable)
                {  
                    m_isEnabled = enable;
                    EnablePasses(enable);
                }

                bool CreatePerPassResources();

                void GetHairGlobalSettings(HairGlobalSettings& hairGlobalSettings) override;
                void SetHairGlobalSettings(const HairGlobalSettings& hairGlobalSettings) override;

            private:
                AZ_DISABLE_COPY_MOVE(HairFeatureProcessor);

                void ClearPasses();

                bool InitPPLLFillPass();
                bool InitPPLLResolvePass();
                bool InitShortCutRenderPasses();
                bool InitComputePass(const Name& passName, bool allowIterations = false);

                void BuildDispatchAndDrawItems(Data::Instance<HairRenderObject> renderObject);

                void EnablePasses(bool enable);

                bool HasHairParentPass(RPI::RenderPipeline* renderPipeline);

                //! The following will serve to register the FP in the Thumbnail system
                AZStd::vector<AZStd::string> m_hairFeatureProcessorRegistryName;

                //! The render pipeline is acquired and set when a pipeline is created or changed
                //! and accordingly the passes and the feature processor are associated.
                //! Notice that scene can contain several pipelines all using the same feature
                //! processor.  On the pass side, it will acquire the scene and request the FP, 
                //! but on the FP side, it will only associate to the latest pass hence such a case
                //! might still be a problem.  If needed, it can be resolved using a map for each
                //! pass name per pipeline.
                RPI::RenderPipeline* m_renderPipeline = nullptr;

                //! The Hair Objects in the scene (one per hair component)
                AZStd::list<Data::Instance<HairRenderObject>> m_hairRenderObjects;

                //! Simulation Compute Passes
                AZStd::unordered_map<Name, Data::Instance<HairSkinningComputePass> > m_computePasses;

                // PPLL Render Passes
                Data::Instance<HairPPLLRasterPass> m_hairPPLLRasterPass = nullptr;
                Data::Instance<HairPPLLResolvePass> m_hairPPLLResolvePass = nullptr;

                // ShortCut Render Passes - special case for the geometry render passes
                Data::Instance<HairShortCutGeometryDepthAlphaPass> m_hairShortCutGeometryDepthAlphaPass = nullptr;
                Data::Instance<HairShortCutGeometryShadingPass> m_hairShortCutGeometryShadingPass = nullptr;

                //--------------------------------------------------------------
                //                      Per Pass Resources 
                //--------------------------------------------------------------
                //! The shared buffer used by all dynamic buffer views for the hair skinning / simulation
                AZStd::unique_ptr<SharedBuffer> m_sharedDynamicBuffer;  // used for the hair data changed between passes.

                //! The constant buffer structure containing an array of all hair objects materials
                //!  to be used by the full screen resolve pass.
                HairUniformBuffer<AMD::TressFXShadeParams> m_hairObjectsMaterialsCB;

                //! PPLL single buffer containing all the PPLL elements
                Data::Instance<RPI::Buffer> m_linkedListNodesBuffer = nullptr;
                //--------------------------------------------------------------

                //! Per frame delta time for the physics simulation - updated every frame
                float m_currentDeltaTime = 0.02f;
                //! flag to disable/enable feature processor adding dispatch calls to compute passes.
                bool m_addDispatchEnabled = true;
                bool m_sharedResourcesCreated = false;
                //! reload / pipeline changes force build dispatches and render items
                bool m_forceRebuildRenderData = false;      
                bool m_forceClearRenderData = false;
                bool m_initialized = false;
                bool m_isEnabled = true;
                bool m_usePPLLRenderTechnique = true;
                static uint32_t s_instanceCount;

                HairGlobalSettings m_hairGlobalSettings;
                AZStd::mutex m_hairGlobalSettingsMutex;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
