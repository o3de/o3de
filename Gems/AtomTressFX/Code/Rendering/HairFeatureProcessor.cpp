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
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Debug/EventTrace.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

// Hair specific
#include <Rendering/HairGlobalSettings.h>
#include <Rendering/HairFeatureProcessor.h>
#include <Rendering/HairBuffersSemantics.h>
#include <Rendering/HairRenderObject.h>

#include <Passes/HairSkinningComputePass.h>

#pragma optimize("", off)

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            const char* HairFeatureProcessor::s_featureProcessorName = "HairFeatureProcessor";
            uint32_t HairFeatureProcessor::s_instanceCount = 0;

            HairFeatureProcessor::HairFeatureProcessor()
            {
                TestSkinningPass = Name{ "HairSkinningComputePassTemplate" }; // [To Do] Adi: Debug pass - remove at polish
                GlobalShapeConstraintsPass = Name{ "HairGlobalShapeConstraintsComputePassTemplate" };
                CalculateStrandDataPass = Name{ "HairCalculateStrandLevelDataComputePassTemplate" };
                VelocityShockPropagationPass = Name{ "HairVelocityShockPropagationComputePassTemplate" };
                LocalShapeConstraintsPass = Name{ "HairLocalShapeConstraintsComputePassTemplate" };
                LengthConstriantsWindAndCollisionPass = Name{ "HairLengthConstraintsWindAndCollisionComputePassTemplate" };
                UpdateFollowHairPass = Name{ "HairUpdateFollowHairComputePassTemplate" };

                ++s_instanceCount;

                // Create right away for everyone to use
                if (!CreatePerPassResources())
                {   // this might not be an error - if the pass system is still empty / minimal
                    //  and these passes are not part of the minimal pipeline, they will not
                    //  be created.
                    AZ_Error("Hair Gem", false, "Failed to create the PerPass Srg.");
                }
            }

            void HairFeatureProcessor::Reflect(ReflectContext* context)
            {
                HairGlobalSettings::Reflect(context);

                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext
                        ->Class<HairFeatureProcessor, RPI::FeatureProcessor>()
                        ->Version(0);
                }
            }

            void HairFeatureProcessor::Activate()
            {
                m_hairFeatureProcessorRegistryName = { "AZ::Render::Hair::HairFeatureProcessor" };

                EnableSceneNotification();
                TickBus::Handler::BusConnect();
            }

            void HairFeatureProcessor::Deactivate()
            {
                DisableSceneNotification();
                TickBus::Handler::BusDisconnect();

                m_sharedDynamicBuffer.reset();
            }

            void HairFeatureProcessor::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                const float MAX_SIMULATION_TIME_STEP = 0.033f;  // Assuming minimal of 30 fps
                m_currentDeltaTime = AZStd::min(deltaTime, MAX_SIMULATION_TIME_STEP);
                for (auto object : m_hairRenderObjects )
                {
                    object->SetFrameDeltaTime(m_currentDeltaTime);
                }
            }

            int HairFeatureProcessor::GetTickOrder()
            {
                return AZ::TICK_PRE_RENDER;
            }

            void HairFeatureProcessor::AddHairRenderObject(Data::Instance<HairRenderObject> renderObject)
            {
                if (!m_initialized)
                {
                    Init();
                }

                m_hairRenderObjects.push_back(renderObject);

                BuildDispatchAndDrawItems(renderObject);

                EnablePasses(true);
            }

            void HairFeatureProcessor::EnablePasses(bool enable)
            {
                // [To Do] Adi: this currently doesn't work as it should (resolve pass gets disabled)
                // and so disabled.
                // For removing overhead, update and include this method
                return;

                for (auto mapIter = m_computePasses.begin() ; mapIter != m_computePasses.end(); ++mapIter)
                {
                    mapIter->second->SetEnabled(enable);
                }
                m_hairPPLLRasterPass->SetEnabled(enable);
                m_hairPPLLResolvePass->SetEnabled(enable);
            }

            bool HairFeatureProcessor::RemoveHairRenderObject(Data::Instance<HairRenderObject> renderObject)
            {
                for ( auto objIter = m_hairRenderObjects.begin() ; objIter != m_hairRenderObjects.end() ; ++objIter )
                {
                    if (objIter->get() == renderObject)
                    {
                        m_hairRenderObjects.erase(objIter);
                        if (m_hairRenderObjects.empty())
                        {
                            EnablePasses(false);
                        }
                        return true;
                    }
                }
                return false;
            }

            void HairFeatureProcessor::UpdateHairSkinning()
            {
                // Copying CPU side m_SimCB content to the GPU buffer (matrices, wind parameters..) 

                for (auto objIter = m_hairRenderObjects.begin(); objIter != m_hairRenderObjects.end(); ++objIter)
                {
                    if (!objIter->get()->IsEnabled())
                    {
                        return;
                    }
                    objIter->get()->Update();
                }
            }

            //! Assumption: the hair is being updated per object before this method is called and
            //!  therefore the parameters that were calculated per object can be directly copied
            //!  without need to recalculate as in the original code.
            //! Make sure there are no more than (currently) 16 hair objects or update dynamic handling.
            //! This DOES NOT do the srg binding since it can be shared but by pass itself when compiling resources.
            //! Originally called 'UpdateShadeParameters' in TressFX
            void HairFeatureProcessor::FillHairMaterialsArray(std::vector<const AMD::TressFXRenderParams*>& renderSettings)
            {
                // Update Render Parameters
                for (int i = 0; i < renderSettings.size(); ++i)
                {
                    AMD::ShadeParams& hairMaterial = m_hairObjectsMaterialsCB->HairShadeParams[i];
                    hairMaterial.FiberRadius = renderSettings[i]->FiberRadius;
                    hairMaterial.ShadowAlpha = renderSettings[i]->ShadowAlpha;
                    hairMaterial.FiberSpacing = renderSettings[i]->FiberSpacing;
                    hairMaterial.HairEx2 = renderSettings[i]->HairEx2;
                    hairMaterial.HairKs2 = renderSettings[i]->HairKs2;
                    hairMaterial.MatKValue = renderSettings[i]->MatKValue;
                    hairMaterial.Roughness = renderSettings[i]->Roughness;
                    hairMaterial.CuticleTilt = renderSettings[i]->CuticleTilt;
                }
            }

            //==========================================================================================
            void HairFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);
                AZ_ATOM_PROFILE_FUNCTION("Hair", "HairFeatureProcessor: Simulate");
                AZ_UNUSED(packet);

                if (m_hairRenderObjects.empty() || (!m_initialized && !Init()))
                {   // there might not be are no render objects yet, indicating that scene data might not be ready
                    // to initialize just yet.
                    return;
                }

                if (m_forceRebuildRenderData)
                {
                    for (auto& hairRenderObject : m_hairRenderObjects)
                    {
                        BuildDispatchAndDrawItems(hairRenderObject);
                    }
                    m_forceRebuildRenderData = false;
                    m_addDispatchEnabled = true;
                }

                // Prepare materials array for the per pass srg
                std::vector<const AMD::TressFXRenderParams*> hairObjectsRenderMaterials;
                uint32_t obj = 0;
                for ( auto objIter = m_hairRenderObjects.begin() ; objIter != m_hairRenderObjects.end(); ++objIter, ++obj )
                {
                    HairRenderObject* renderObject = objIter->get();
                    if (!renderObject->IsEnabled())
                    {
                        continue;
                    }
                    renderObject->Update();

                    // [To Do] Adi: the next section can be skipped for now - only distance related parameters
                    // should change or when parameters are being changed on the editor side.
//                         float Distance = sqrtf( m_activeScene.scene->GetCameraPos().x * m_activeScene.scene->GetCameraPos().x +
//                                                  m_activeScene.scene->GetCameraPos().y * m_activeScene.scene->GetCameraPos().y +
//                                                  m_activeScene.scene->GetCameraPos().z * m_activeScene.scene->GetCameraPos().z);
//                    objIter->get()->UpdateRenderingParameters(
//                              renderingSettings, m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL, m_deltaTime, Distance);

                    // this will be used for the constant buffer
                    hairObjectsRenderMaterials.push_back(renderObject->GetHairRenderParams());
                }
                FillHairMaterialsArray(hairObjectsRenderMaterials);

//            HairFeatureProcessorNotificationBus::Broadcast(&HairFeatureProcessorNotificationBus::Events::OnUpdateSkinningMatrices);
            }


            void HairFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);
                AZ_ATOM_PROFILE_FUNCTION("Hair", "HairFeatureProcessor: Render");

                if (!m_initialized && !Init())
                {
                    return;
                }

                // Adi: should this be moved to 'OnBeginPrepareRender' / 'SImulate'?
                // [To Do] Adi: missing culling scheme - setup the hair culling work group
                // (to be re-used for each view). See SkinnedMeshFeatureProcessor::Render for more details
                for (auto objIter = m_hairRenderObjects.begin(); objIter != m_hairRenderObjects.end(); ++objIter)
                {
                    HairRenderObject* renderObject = objIter->get();
                    if (!renderObject->IsEnabled())
                    {
                        continue;
                    }

                    // Compute pases
                    if (m_addDispatchEnabled)
                    {
                        for (auto mapIter = m_computePasses.begin(); mapIter != m_computePasses.end(); ++mapIter)
                        {
                            mapIter->second->AddDispatchItem(renderObject);
                        }
                    }

                    // Render / Raster Passes - these might be enabled even is simulation is on pause / reload
                    m_hairPPLLRasterPass->AddDrawPacket(renderObject);
                }
            }

            void HairFeatureProcessor::ClearPasses()
            {
                m_initialized = false;      // Avoid simulation or render
                m_computePasses.clear();
                m_hairPPLLRasterPass = nullptr;
                m_hairPPLLResolvePass = nullptr;

                // [To Do] Adi: This is not enough!
                // here we need to signal all pases to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
                m_forceClearRenderData = true;  // [To Do] Adi: implement the use case
            }

            void HairFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr pipeline)
            {
                Init();
                // [To Do] Adi: This is not enough!
                // here we need to signal all pases to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
            }

            void HairFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
            {
                ClearPasses();
            }

            void HairFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
            {
                Init();
                // [To Do] Adi: This is not enough!
                // here we need to signal all pases to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
            }
            
            void HairFeatureProcessor::OnBeginPrepareRender()
            {
                RPI::FeatureProcessor::OnBeginPrepareRender();
            }

            void HairFeatureProcessor::OnEndPrepareRender()
            {
                RPI::FeatureProcessor::OnEndPrepareRender();
            }

            bool HairFeatureProcessor::Init()
            {
                ClearPasses();

                // Compute Passes - populate the passes map
                bool resultSuccess = InitComputePass(TestSkinningPass);
                resultSuccess &= InitComputePass(GlobalShapeConstraintsPass);
                resultSuccess &= InitComputePass(CalculateStrandDataPass);
                resultSuccess &= InitComputePass(VelocityShockPropagationPass);
                resultSuccess &= InitComputePass(LocalShapeConstraintsPass, true);  // restore shape over several iterations
                resultSuccess &= InitComputePass(LengthConstriantsWindAndCollisionPass);
                resultSuccess &= InitComputePass(UpdateFollowHairPass);

                // Rendering Passes
                resultSuccess &= InitPPLLFillPass();
                resultSuccess &= InitPPLLResolvePass();

                // No need to have the passes enabled if no hair object was added 
                EnablePasses(false);

                m_initialized = resultSuccess;

                // this might not be an error - if the pass system is still empty / minimal
                //  and these passes are not part of the minimal pipeline, they will not
                //  be created.
                AZ_Error("Hair Gem", resultSuccess, "Passes could not be retrieved.");

                return m_initialized;
            }

            bool HairFeatureProcessor::CreatePerPassResources()
            {
                if (m_sharedResourcesCreated)
                {
                    return true;
                }

                SrgBufferDescriptor descriptor;
                AZStd::string instanceNumber = AZStd::to_string(s_instanceCount);

                // Shared buffer - this is a persistent buffer that needs to be created manually.
                {
                    AZStd::vector<SrgBufferDescriptor> hairDynamicDescriptors;
                    DynamicHairData::PrepareSrgDescriptors(hairDynamicDescriptors, 1, 1);
                    Name sharedBufferName = Name{ "HairSharedDynamicBuffer" + instanceNumber };
                    if (!SharedBufferInterface::Get())
                    {   // SInce there can be several pipelines, allocate the shared buffer only for the
                        // first one and from that moment on it will be used through its interface
                        m_sharedDynamicBuffer = AZStd::make_unique<SharedBuffer>(sharedBufferName.GetCStr(), hairDynamicDescriptors);
                    }
                }

                // PPLL nodes buffer
                {
                    descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                        Name{ "LinkedListNodesPPLL" + instanceNumber}, Name{ "m_linkedListNodes" }, 0, 0
                    );
                    m_linkedListNodesBuffer = UtilityClass::CreateBuffer("Hair Gem", descriptor, nullptr);
                    if (!m_linkedListNodesBuffer)
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", descriptor.m_bufferName.GetCStr());
                        return false;
                    }
                }

                // PPLL index counter
                {
                    descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        sizeof(uint32_t), 1,
                        Name{ "LinkedListCounterPPLL" + instanceNumber }, Name{ "m_linkedListCounter" }, 0, 0
                    );
                    m_linkedListCounterBuffer = UtilityClass::CreateBuffer("Hair Gem", descriptor, nullptr );
                    if (!m_linkedListCounterBuffer)
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", descriptor.m_bufferName.GetCStr());
                        return false;
                    }
                }
                m_sharedResourcesCreated = true;
                return true;
            }

            // Adi: is this required? if data driven via the MainPipeline.pass the answer is probably no?!
            bool HairFeatureProcessor::InitComputePass(const Name& passName, bool allowIterations)
            {
                m_computePasses[passName] = nullptr;

                RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();
                if (passSystem->HasPassesForTemplateName(passName))
                {
                    auto& desiredPasses = passSystem->GetPassesForTemplateName(passName);
                    if (!desiredPasses.empty() && desiredPasses[0])
                    {
                        m_computePasses[passName] = static_cast<HairSkinningComputePass*>(desiredPasses[0]);
                        m_computePasses[passName]->SetFeatureProcessor(this);
                        m_computePasses[passName]->SetAllowIterations(allowIterations);
                    }
                    else
                    {
                        AZ_Error("Hair Gem", false,
                            "%s does not have any valid passes. Check your game project's .pass assets.",
                            passName.GetCStr());
                        return false;
                    }
                }
                else
                {
                    AZ_Error("Hair Gem", false,
                        "Failed to find passes for %s. Check your game project's .pass assets.",
                        passName.GetCStr());
                    return false;
                }
                return true;
            }

            // Adi: is this required? if data driven via the MainPipeline.pass the answer is probably no?!
            bool HairFeatureProcessor::InitPPLLFillPass()
            {
                m_hairPPLLRasterPass = nullptr;   // reset it to null, just in case it fails to load the assets properly

                RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();
                if (passSystem->HasPassesForTemplateName(AZ::Name{HairPPLLRasterPassTemplateName}))
                {
                    auto& desiredPasses = passSystem->GetPassesForTemplateName(AZ::Name{HairPPLLRasterPassTemplateName});

                    // For now, assume one skinning pass
                    if (!desiredPasses.empty() && desiredPasses[0])
                    {
                        m_hairPPLLRasterPass = static_cast<HairPPLLRasterPass*>(desiredPasses[0]);
                        m_hairPPLLRasterPass->SetFeatureProcessor(this);                  }
                    else
                    {
                        AZ_Error("Hair Gem", false, "HairPPLLRasterPassTemplate does not have any valid passes. Check your game project's .pass assets.");
                        return false;
                    }
                }
                else
                {
                    AZ_Error("Hair Gem", false, "Failed to find passes for HairPPLLRasterPassTemplate. Check your game project's .pass assets.");
                    return false;
                }
                return true;
            }


            // Adi: is this required? if data driven via the MainPipeline.pass the answer is probably no?!
            bool HairFeatureProcessor::InitPPLLResolvePass()
            {
                m_hairPPLLResolvePass = nullptr;   // reset it to null, just in case it fails to load the assets properly

                RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();
                if (passSystem->HasPassesForTemplateName(AZ::Name{HairPPLLResolvePassTemplateName}))
                {
                    auto& desiredPasses = passSystem->GetPassesForTemplateName(AZ::Name{HairPPLLResolvePassTemplateName});

                    // For now, assume one skinning pass
                    if (!desiredPasses.empty() && desiredPasses[0])
                    {
                         m_hairPPLLResolvePass = static_cast<HairPPLLResolvePass*>(desiredPasses[0]);
                         m_hairPPLLResolvePass->SetFeatureProcessor(this);
                    }
                    else
                    {
                        AZ_Error("Hair Gem", false, "HairPPLLResolvePassTemplate does not have any valid passes. Check your game project's .pass assets.");
                        return false;
                    }
                }
                else
                {
                    AZ_Error("Hair Gem", false, "Failed to find passes for HairPPLLResolvePassTemplate. Check your game project's .pass assets.");
                    return false;
                }
                return true;
            }

            void HairFeatureProcessor::BuildDispatchAndDrawItems(Data::Instance<HairRenderObject> renderObject)
            {
                HairRenderObject* renderObjectPtr = renderObject.get();

                // Dispatches for Compute passes
                m_computePasses[TestSkinningPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[GlobalShapeConstraintsPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[CalculateStrandDataPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_STRAND);
                m_computePasses[VelocityShockPropagationPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[LocalShapeConstraintsPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_STRAND);
                m_computePasses[LengthConstriantsWindAndCollisionPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[UpdateFollowHairPass]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);

                // Render / Raster pass - adding the object will schedule Srgs binding
                // and DrawItem build.
                m_hairPPLLRasterPass->SchedulePacketBuild(renderObjectPtr);
            }

            Data::Instance<HairSkinningComputePass> HairFeatureProcessor::GetHairSkinningComputegPass()
            {
                if (!m_computePasses[TestSkinningPass])
                {   
                    Init();
                }
                return m_computePasses[TestSkinningPass];
            }

            Data::Instance<HairPPLLRasterPass> HairFeatureProcessor::GetHairPPLLRasterPass()
            {
                if (!m_hairPPLLRasterPass)
                {
                    Init();
                }
                return m_hairPPLLRasterPass;
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ

#pragma optimize("", on)
