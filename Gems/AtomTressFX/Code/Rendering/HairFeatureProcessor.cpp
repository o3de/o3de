/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            uint32_t HairFeatureProcessor::s_instanceCount = 0;

            HairFeatureProcessor::HairFeatureProcessor()
            {
                HairParentPassName = Name{ "HairParentPass" };

                HairPPLLRasterPassName = Name{ "HairPPLLRasterPass" };
                HairPPLLResolvePassName = Name{ "HairPPLLResolvePass" };

                GlobalShapeConstraintsPassName = Name{ "HairGlobalShapeConstraintsComputePass" };
                CalculateStrandDataPassName = Name{ "HairCalculateStrandLevelDataComputePass" };
                VelocityShockPropagationPassName = Name{ "HairVelocityShockPropagationComputePass" };
                LocalShapeConstraintsPassName = Name{ "HairLocalShapeConstraintsComputePass" };
                LengthConstriantsWindAndCollisionPassName = Name{ "HairLengthConstraintsWindAndCollisionComputePass" };
                UpdateFollowHairPassName = Name{ "HairUpdateFollowHairComputePass" };

                ++s_instanceCount;

                if (!CreatePerPassResources())
                {   // this might not be an error - if the pass system is still empty / minimal
                    //  and these passes are not part of the minimal pipeline, they will not
                    //  be created.
                    AZ_Error("Hair Gem", false, "Failed to create the hair shared buffer resource");
                }
            }

            HairFeatureProcessor::~HairFeatureProcessor()
            {
                m_linkedListNodesBuffer.reset();
                m_sharedDynamicBuffer.reset();
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
                HairGlobalSettingsRequestBus::Handler::BusConnect();
            }

            void HairFeatureProcessor::Deactivate()
            {
                DisableSceneNotification();
                TickBus::Handler::BusDisconnect();
                HairGlobalSettingsRequestBus::Handler::BusDisconnect();
            }

            void HairFeatureProcessor::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                const float MAX_SIMULATION_TIME_STEP = 0.033f;  // Assuming minimal of 30 fps
                m_currentDeltaTime = AZStd::min(deltaTime, MAX_SIMULATION_TIME_STEP);
                for (auto& object : m_hairRenderObjects)
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
                    Init(m_renderPipeline);
                }

                m_hairRenderObjects.push_back(renderObject);

                BuildDispatchAndDrawItems(renderObject);

                EnablePasses(true);
            }

            void HairFeatureProcessor::EnablePasses(bool enable)
            {
                if (!m_initialized)
                {
                    return;
                }

                for (auto& [passName, pass] : m_computePasses)
                {
                    pass->SetEnabled(enable);
                }

                m_hairPPLLRasterPass->SetEnabled(enable);
                m_hairPPLLResolvePass->SetEnabled(enable);
            }

            bool HairFeatureProcessor::RemoveHairRenderObject(Data::Instance<HairRenderObject> renderObject)
            {
                for (auto objIter = m_hairRenderObjects.begin(); objIter != m_hairRenderObjects.end(); ++objIter)
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
                AZ_PROFILE_FUNCTION(AzRender);
                AZ_UNUSED(packet);

                if (m_hairRenderObjects.empty())
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
                uint32_t objectIndex = 0;
                for (auto& renderObject : m_hairRenderObjects)
                {
                    if (!renderObject->IsEnabled())
                    {
                        continue;
                    }

                    renderObject->SetRenderIndex(objectIndex);

                    // [To Do] Hair - update the following parameters for dynamic LOD control
                    // should change or when parameters are being changed on the editor side.
//                         float Distance = sqrtf( m_activeScene.scene->GetCameraPos().x * m_activeScene.scene->GetCameraPos().x +
//                                                  m_activeScene.scene->GetCameraPos().y * m_activeScene.scene->GetCameraPos().y +
//                                                  m_activeScene.scene->GetCameraPos().z * m_activeScene.scene->GetCameraPos().z);
                    const float distanceFromCamera = 1.0f;      // fixed distance until LOD mechanism is worked on
                    const float updateShadows = false;          // same here - currently cheap self shadow approx
                    renderObject->UpdateRenderingParameters( nullptr, RESERVED_PIXELS_FOR_OIT, distanceFromCamera, updateShadows);

                    // this will be used in the constant buffer to set the material array used by the resolve pass
                    hairObjectsRenderMaterials.push_back(renderObject->GetHairRenderParams());

                    // The data update for the GPU bind - this should be the very last thing done after the
                    // data has been read and / or altered on the CPU side.
                    renderObject->Update();
                    ++objectIndex;
                }

                FillHairMaterialsArray(hairObjectsRenderMaterials);
            }


            void HairFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
            {
                AZ_PROFILE_FUNCTION(AzRender);

                if (!m_initialized || !m_addDispatchEnabled)
                {   // Skip adding dispatches / Draw packets for this frame until initialized and the shaders are ready
                    return;
                }

                // [To Do] - no culling scheme applied yet.
                // Possibly setup the hair culling work group to be re-used for each view.
                // See SkinnedMeshFeatureProcessor::Render for more details
                
                // Add dispatch per hair object per Compute passes
                for (auto& [passName, pass] : m_computePasses)
                {
                    pass->AddDispatchItems(m_hairRenderObjects);
                }

                // Add all hair objects to the Render / Raster Pass
                m_hairPPLLRasterPass->AddDrawPackets(m_hairRenderObjects);
            }

            void HairFeatureProcessor::ClearPasses()
            {
                m_initialized = false;      // Avoid simulation or render
                m_computePasses.clear();
                m_hairPPLLRasterPass = nullptr;
                m_hairPPLLResolvePass = nullptr;

                // Mark for all passes to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
                m_forceClearRenderData = true;
            }

            void HairFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline)
            {
                // Proceed only if this is the main pipeline that contains the parent pass
                if (!renderPipeline.get()->GetRootPass()->FindPassByNameRecursive(HairParentPassName))
                {
                    return;
                }

                Init(renderPipeline.get());

                // Mark for all passes to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
            }

            void HairFeatureProcessor::OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline)
            {
                // Proceed only if this is the main pipeline that contains the parent pass
                if (!renderPipeline->GetRootPass()->FindPassByNameRecursive(HairParentPassName))
                {
                    return;
                }

                m_renderPipeline = nullptr;
                ClearPasses();
            }

            void HairFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
            {
                // Proceed only if this is the main pipeline that contains the parent pass
                if (!renderPipeline->GetRootPass()->FindPassByNameRecursive(HairParentPassName))
                {
                    return;
                }

                Init(renderPipeline);

                // Mark for all passes to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
            }

            bool HairFeatureProcessor::Init(RPI::RenderPipeline* renderPipeline)
            {
                m_renderPipeline = renderPipeline;

                ClearPasses();

                // Compute Passes - populate the passes map
                bool resultSuccess = InitComputePass(GlobalShapeConstraintsPassName);
                resultSuccess &= InitComputePass(CalculateStrandDataPassName);
                resultSuccess &= InitComputePass(VelocityShockPropagationPassName);
                resultSuccess &= InitComputePass(LocalShapeConstraintsPassName, true);  // restore shape over several iterations
                resultSuccess &= InitComputePass(LengthConstriantsWindAndCollisionPassName);
                resultSuccess &= InitComputePass(UpdateFollowHairPassName);

                // Rendering Passes
                resultSuccess &= InitPPLLFillPass();
                resultSuccess &= InitPPLLResolvePass();

                m_initialized = resultSuccess;

                // Don't enable passes if no hair object was added yet (depending on activation order)
                if (m_initialized && m_hairRenderObjects.empty())
                {
                    EnablePasses(false);
                }

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
                    if (!HairSharedBufferInterface::Get())
                    {   // Since there can be several pipelines, allocate the shared buffer only for the
                        // first one and from that moment on it will be used through its interface
                        m_sharedDynamicBuffer = AZStd::make_unique<SharedBuffer>(sharedBufferName.GetCStr(), hairDynamicDescriptors);
                    }
                }

                // PPLL nodes buffer
                {
                    descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                        Name{ "LinkedListNodesPPLL" + instanceNumber }, Name{ "m_linkedListNodes" }, 0, 0
                    );
                    m_linkedListNodesBuffer = UtilityClass::CreateBuffer("Hair Gem", descriptor, nullptr);
                    if (!m_linkedListNodesBuffer)
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", descriptor.m_bufferName.GetCStr());
                        return false;
                    }
                }

                m_sharedResourcesCreated = true;
                return true;
            }

            void HairFeatureProcessor::GetHairGlobalSettings(HairGlobalSettings& hairGlobalSettings)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_hairGlobalSettingsMutex);
                hairGlobalSettings = m_hairGlobalSettings;
            }

            void HairFeatureProcessor::SetHairGlobalSettings(const HairGlobalSettings& hairGlobalSettings)
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_hairGlobalSettingsMutex);
                    m_hairGlobalSettings = hairGlobalSettings;
                }
                HairGlobalSettingsNotificationBus::Broadcast(&HairGlobalSettingsNotifications::OnHairGlobalSettingsChanged, m_hairGlobalSettings);
            }

            bool HairFeatureProcessor::InitComputePass(const Name& passName, bool allowIterations)
            {
                m_computePasses[passName] = nullptr;
                if (!m_renderPipeline)
                {
                    AZ_Error("Hair Gem", false, "%s does NOT have render pipeline set yet", passName.GetCStr());
                    return false;
                }

                RPI::Ptr<RPI::Pass> desiredPass = m_renderPipeline->GetRootPass()->FindPassByNameRecursive(passName);
                if (desiredPass)
                {
                    m_computePasses[passName] = static_cast<HairSkinningComputePass*>(desiredPass.get());
                    m_computePasses[passName]->SetFeatureProcessor(this);
                    m_computePasses[passName]->SetAllowIterations(allowIterations);
                }
                else
                {
                    AZ_Error("Hair Gem", false,
                        "%s does not exist in this pipeline. Check your game project's .pass assets.",
                        passName.GetCStr());
                    return false;
                }

                return true;
            }

            bool HairFeatureProcessor::InitPPLLFillPass()
            {
                m_hairPPLLRasterPass = nullptr;   // reset it to null, just in case it fails to load the assets properly
                if (!m_renderPipeline)
                {
                    AZ_Error("Hair Gem", false, "Hair Fill Pass does NOT have render pipeline set yet");
                    return false;
                }

                RPI::Ptr<RPI::Pass> desiredPass = m_renderPipeline->GetRootPass()->FindPassByNameRecursive(HairPPLLRasterPassName);
                if (desiredPass)
                {
                    m_hairPPLLRasterPass = static_cast<HairPPLLRasterPass*>(desiredPass.get());
                    m_hairPPLLRasterPass->SetFeatureProcessor(this);
                }
                else
                {
                    AZ_Error("Hair Gem", false, "HairPPLLRasterPass does not have any valid passes. Check your game project's .pass assets.");
                    return false;
                }
                return true;
            }

            bool HairFeatureProcessor::InitPPLLResolvePass()
            {
                m_hairPPLLResolvePass = nullptr;   // reset it to null, just in case it fails to load the assets properly
                if (!m_renderPipeline)
                {
                    AZ_Error("Hair Gem", false, "Hair Fill Pass does NOT have render pipeline set yet");
                    return false;
                }

                RPI::Ptr<RPI::Pass> desiredPass = m_renderPipeline->GetRootPass()->FindPassByNameRecursive(HairPPLLResolvePassName);
                if (desiredPass)
                {
                    m_hairPPLLResolvePass = static_cast<HairPPLLResolvePass*>(desiredPass.get());
                    m_hairPPLLResolvePass->SetFeatureProcessor(this);
                }
                else
                {
                    AZ_Error("Hair Gem", false, "HairPPLLResolvePassTemplate does not have valid passes. Check your game project's .pass assets.");
                    return false;
                }
                return true;
            }

            void HairFeatureProcessor::BuildDispatchAndDrawItems(Data::Instance<HairRenderObject> renderObject)
            {
                HairRenderObject* renderObjectPtr = renderObject.get();

                // Dispatches for Compute passes
                m_computePasses[GlobalShapeConstraintsPassName]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[CalculateStrandDataPassName]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_STRAND);
                m_computePasses[VelocityShockPropagationPassName]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[LocalShapeConstraintsPassName]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_STRAND);
                m_computePasses[LengthConstriantsWindAndCollisionPassName]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);
                m_computePasses[UpdateFollowHairPassName]->BuildDispatchItem(
                    renderObjectPtr, DispatchLevel::DISPATCHLEVEL_VERTEX);

                // Render / Raster pass - adding the object will schedule Srgs binding
                // and DrawItem build.
                m_hairPPLLRasterPass->SchedulePacketBuild(renderObjectPtr);
            }

            Data::Instance<HairSkinningComputePass> HairFeatureProcessor::GetHairSkinningComputegPass()
            {
                if (!m_computePasses[GlobalShapeConstraintsPassName])
                {
                    Init(m_renderPipeline);
                }
                return m_computePasses[GlobalShapeConstraintsPassName];
            }

            Data::Instance<HairPPLLRasterPass> HairFeatureProcessor::GetHairPPLLRasterPass()
            {
                if (!m_hairPPLLRasterPass)
                {
                    Init(m_renderPipeline);
                }
                return m_hairPPLLRasterPass;
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ
