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

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

// Hair specific
#include <TressFX/TressFXConstantBuffers.h>

#include <Passes/HairSkinningComputePass.h>
#include <Passes/HairPPLLRasterPass.h>
#include <Passes/HairPPLLResolvePass.h>

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
            static const size_t PPLL_NODE_SIZE = 16;
            // Adi: replace the following with the actual screen dimensions.
            static const size_t AVE_FRAGS_PER_PIXEL = 24;
            static const size_t SCREEN_WIDTH = 1920;
            static const size_t SCREEN_HEIGHT = 1080;
            static const size_t RESERVED_PIXELS_FOR_OIT = SCREEN_WIDTH * SCREEN_HEIGHT * AVE_FRAGS_PER_PIXEL;

            class HairSkinningPass;
            class HairRenderObject;

            class HairFeatureProcessor final
                : public RPI::FeatureProcessor
                , public HairGlobalSettingsRequestBus::Handler
                , private AZ::TickBus::Handler
            {
                Name TestSkinningPass;
                Name GlobalShapeConstraintsPass;
                Name CalculateStrandDataPass;
                Name VelocityShockPropagationPass;
                Name LocalShapeConstraintsPass;
                Name LengthConstriantsWindAndCollisionPass;
                Name UpdateFollowHairPass;

            public:
                AZ_RTTI(AZ::Render::Hair::HairFeatureProcessor, "{5F9DDA81-B43F-4E30-9E56-C7C3DC517A4C}", RPI::FeatureProcessor);
                AZ_FEATURE_PROCESSOR(HairFeatureProcessor);

                static void Reflect(AZ::ReflectContext* context);

                HairFeatureProcessor();
                virtual ~HairFeatureProcessor() = default;


                void UpdateHairSkinning();

                bool Init();

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
                void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
                void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
                void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

                void OnBeginPrepareRender() override;
                void OnEndPrepareRender() override;

                Data::Instance<HairSkinningComputePass> GetHairSkinningComputegPass();
                Data::Instance<HairPPLLRasterPass> GetHairPPLLRasterPass();

                //! Update the hair objects materials array.
                void FillHairMaterialsArray(std::vector<const AMD::TressFXRenderParams*>& renderSettings);

                Data::Instance<RPI::Buffer> GetPerPixelListBuffer() { return m_linkedListNodesBuffer; }
                Data::Instance<RPI::Buffer> GetPerPixelCounterBuffer() { return m_linkedListCounterBuffer;  }
                HairUniformBuffer<AMD::TressFXShadeParams>& GetMaterialsArray() { return m_hairObjectsMaterialsCB;  }

                void ForceRebuildRenderData() { m_forceRebuildRenderData = true; }
                void SetAddDispatchEnable(bool enable) { m_addDispatchEnabled = enable; }
                void SetEnable(bool enable)
                {   // [To Do] Adi: test is this is all that might be required.
                    m_isEnabled = enable;
                    EnablePasses(enable);
                }

                bool CreatePerPassResources();

                const HairGlobalSettings& GetHairGlobalSettings() const override;
                void SetHairGlobalSettings(const HairGlobalSettings& hairGlobalSettings) override;

            private:
                AZ_DISABLE_COPY_MOVE(HairFeatureProcessor);

                void ClearPasses();

                bool InitPPLLFillPass();
                bool InitPPLLResolvePass();
                bool InitComputePass(const Name& passName, bool allowIterations = false);

                void BuildDispatchAndDrawItems(Data::Instance<HairRenderObject> renderObject);

                void EnablePasses(bool enable);

                static const char* s_featureProcessorName;

                //! The following will serve to register the FP in the Thumbnail system
                AZStd::vector<AZStd::string> m_hairFeatureProcessorRegistryName;

                //! The Hair Objects in the scene (one per hair component)
                AZStd::list<Data::Instance<HairRenderObject>> m_hairRenderObjects;

                //! Simulation Compute Passes
                AZStd::unordered_map<Name, Data::Instance<HairSkinningComputePass> > m_computePasses;

                // Render Passes
                Data::Instance<HairPPLLRasterPass> m_hairPPLLRasterPass = nullptr;
                Data::Instance<HairPPLLResolvePass> m_hairPPLLResolvePass = nullptr;

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
                //! shared counter representing the next free index in the buffer
                //! 
                Data::Instance<RPI::Buffer> m_linkedListCounterBuffer = nullptr;
                //--------------------------------------------------------------

                float m_currentDeltaTime = 0.02f;    // per frame delta time for the physics simulation.
                bool m_addDispatchEnabled = true; // flag to disable/enable feature processor adding dispatch calls to compute passes.
                bool m_sharedResourcesCreated = false;
                bool m_forceRebuildRenderData = false;      // reload / pipeline changes force build dispatches and render items
                bool m_forceClearRenderData = false;
                bool m_initialized = false;
                bool m_isEnabled = true;
                static uint32_t s_instanceCount;

                HairGlobalSettings m_hairGlobalSettings;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
