/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
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
                m_usePPLLRenderTechnique = false;   // Use the ShortCut rendering technique

                HairParentPassName = Name{ "HairParentPass" };

                // Hair Skinning and Simulation Compute passes
                GlobalShapeConstraintsPassName = Name{ "HairGlobalShapeConstraintsComputePass" };
                CalculateStrandDataPassName = Name{ "HairCalculateStrandLevelDataComputePass" };
                VelocityShockPropagationPassName = Name{ "HairVelocityShockPropagationComputePass" };
                LocalShapeConstraintsPassName = Name{ "HairLocalShapeConstraintsComputePass" };
                LengthConstriantsWindAndCollisionPassName = Name{ "HairLengthConstraintsWindAndCollisionComputePass" };
                UpdateFollowHairPassName = Name{ "HairUpdateFollowHairComputePass" };

                // PPLL render technique pases
                HairPPLLRasterPassName = Name{ "HairPPLLRasterPass" };
                HairPPLLResolvePassName = Name{ "HairPPLLResolvePass" };

                // ShortCut render technique pases
                HairShortCutGeometryDepthAlphaPassName = Name{ "HairShortCutGeometryDepthAlphaPass" };
                HairShortCutResolveDepthPassName = Name{ "HairShortCutResolveDepthPass" };
                HairShortCutGeometryShadingPassName = Name{ "HairShortCutGeometryShadingPass" };
                HairShortCutResolveColorPassName = Name{ "HairShortCutResolveColorPass" };

                ++s_instanceCount;
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
                        ->Version(1);
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
                m_hairPassRequestAsset.Reset();
                DisableSceneNotification();
                TickBus::Handler::BusDisconnect();
                HairGlobalSettingsRequestBus::Handler::BusDisconnect();
            }

            void HairFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
            {
                AddHairParentPass(renderPipeline);
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

                // Adding the object will schedule Srgs binding and the DrawItem build for the geometry passes.
                BuildDispatchAndDrawItems(renderObject);

                EnablePasses(true);
            }

            void HairFeatureProcessor::EnablePasses(bool enable)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(HairParentPassName, GetParentScene());
                RPI::Pass* pass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
                if (pass)
                {
                    pass->SetEnabled(enable);
                }
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
                for (auto& hairRenderObject : m_hairRenderObjects)
                {
                    if (hairRenderObject->IsEnabled())
                    {
                        hairRenderObject->Update();
                    }
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
                {   // In the case of a force build, schedule Srgs binding and the DrawItem build for
                    // the geometry passes of all existing hair objects.
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

                // Skip adding draw or dispath items if there it no hair render objects
                if (m_hairRenderObjects.size() == 0)
                {
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

                if (m_usePPLLRenderTechnique)
                {
                    // Add all hair objects to the Render / Raster Pass
                    m_hairPPLLRasterPass->AddDrawPackets(m_hairRenderObjects);
                }
                else
                {
                    m_hairShortCutGeometryDepthAlphaPass->AddDrawPackets(m_hairRenderObjects);
                    m_hairShortCutGeometryShadingPass->AddDrawPackets(m_hairRenderObjects);
                }
            }

            void HairFeatureProcessor::ClearPasses()
            {
                m_initialized = false;      // Avoid simulation or render
                m_computePasses.clear();

                // PPLL geometry and resolve full screen passes
                m_hairPPLLRasterPass = nullptr;
                m_hairPPLLResolvePass = nullptr;

                // ShortCut passes - Special handling of geometry passes only, and using the regular
                // full screen pass for the resolve
                m_hairShortCutGeometryDepthAlphaPass = nullptr;
                m_hairShortCutGeometryShadingPass = nullptr;

                // Mark for all passes to evacuate their render data and recreate it.
                m_forceRebuildRenderData = true;
                m_forceClearRenderData = true;
            }

            bool HairFeatureProcessor::HasHairParentPass(RPI::RenderPipeline* renderPipeline)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(HairParentPassName, renderPipeline);
                RPI::Pass* pass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
                return pass ? true : false;
            }

            bool HairFeatureProcessor::AddHairParentPass(RPI::RenderPipeline* renderPipeline)
            {
                if (HasHairParentPass(renderPipeline))
                {
                    CreatePerPassResources();
                    return true;
                }
                // check if the reference pass of insert position exist
                Name opaquePassName = Name("OpaquePass");
                if (renderPipeline->FindFirstPass(opaquePassName) == nullptr)
                {
                    AZ_Warning("HairFeatureProcessor", false, "Can't find %s in the render pipeline. Atom TressFX won't be rendered", opaquePassName.GetCStr());
                    return false;
                }

                const char* passRequestAssetFilePath = "Passes/AtomTressFX_PassRequest.azasset";
                m_hairPassRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                    passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
                const AZ::RPI::PassRequest* passRequest = nullptr;
                if (m_hairPassRequestAsset->IsReady())
                {
                    passRequest = m_hairPassRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
                }
                if (!passRequest)
                {
                    AZ_Error("AtomTressFx", false, "Failed to add hair parent pass. Can't load PassRequest from %s", passRequestAssetFilePath);
                    return false;
                }

                m_usePPLLRenderTechnique = passRequest->m_templateName == AZ::Name("HairParentPassTemplate");

                 // Create the pass
                RPI::Ptr<RPI::Pass> hairParentPass  = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
                if (!hairParentPass)
                {
                    AZ_Error("AtomTressFx", false, "Create hair parent pass from pass request failed",
                        renderPipeline->GetId().GetCStr());
                    return false;
                }

                // Add the pass to render pipeline
                bool success = renderPipeline->AddPassAfter(hairParentPass, Name("OpaquePass"));
                // only create pass resources if it was success
                if (success)
                {
                    CreatePerPassResources();
                }
                else
                {
                    AZ_Error("AtomTressFx", false, "Add the hair parent pass to render pipeline [%s] failed",
                        renderPipeline->GetId().GetCStr());
                }
                return success;
            }

            void HairFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline,
                RPI::SceneNotification::RenderPipelineChangeType changeType)
            {
                // Proceed only if this is the main pipeline that contains the parent pass
                if (!HasHairParentPass(renderPipeline))
                {
                    return;
                }

                if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                    || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
                {
                    Init(renderPipeline);

                    // Mark for all passes to evacuate their render data and recreate it.
                    m_forceRebuildRenderData = true;
                }
                else if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
                {
                    m_renderPipeline = nullptr;
                    ClearPasses();
                }
            }

            bool HairFeatureProcessor::Init(RPI::RenderPipeline* renderPipeline)
            {
                m_renderPipeline = renderPipeline;

                ClearPasses();

                if (!m_renderPipeline)
                {
                    AZ_Error("Hair Gem", false, "HairFeatureProcessor does NOT have render pipeline set yet");
                    return false;
                }

                // Compute Passes - populate the passes map
                bool resultSuccess = InitComputePass(GlobalShapeConstraintsPassName);
                resultSuccess &= InitComputePass(CalculateStrandDataPassName);
                resultSuccess &= InitComputePass(VelocityShockPropagationPassName);
                resultSuccess &= InitComputePass(LocalShapeConstraintsPassName, true);  // restore shape over several iterations
                resultSuccess &= InitComputePass(LengthConstriantsWindAndCollisionPassName);
                resultSuccess &= InitComputePass(UpdateFollowHairPassName);

                // Rendering Passes
                if (m_usePPLLRenderTechnique)
                {
                    resultSuccess &= InitPPLLFillPass();
                    resultSuccess &= InitPPLLResolvePass();
                }
                else
                {
                    resultSuccess &= InitShortCutRenderPasses();
                }

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
                SrgBufferDescriptor descriptor;
                AZStd::string instanceNumber = AZStd::to_string(s_instanceCount);

                // Shared buffer - this is a persistent buffer that needs to be created manually.
                if (!m_sharedDynamicBuffer)
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

                // PPLL nodes buffer - created only if the PPLL technique is used
                if (m_usePPLLRenderTechnique && !m_linkedListNodesBuffer)
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

                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
                RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
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
                
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(HairPPLLRasterPassName, m_renderPipeline);
                RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
                if (desiredPass)
                {
                    m_hairPPLLRasterPass = static_cast<HairPPLLRasterPass*>(desiredPass.get());
                    m_hairPPLLRasterPass->SetFeatureProcessor(this);
                }
                else
                {
                    AZ_Error("Hair Gem", false, "HairPPLLRasterPass cannot be found. Check your game project's .pass assets.");
                    return false;
                }
                return true;
            }

            bool HairFeatureProcessor::InitPPLLResolvePass()
            {
                m_hairPPLLResolvePass = nullptr;   // reset it to null, just in case it fails to load the assets properly

                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(HairPPLLResolvePassName, m_renderPipeline);
                RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
                if (desiredPass)
                {
                    m_hairPPLLResolvePass = static_cast<HairPPLLResolvePass*>(desiredPass.get());
                    m_hairPPLLResolvePass->SetFeatureProcessor(this);
                }
                else
                {
                    AZ_Error("Hair Gem", false, "HairPPLLResolvePass cannot be found. Check your game project's .pass assets.");
                    return false;
                }
                return true;
            }

            //! Set the two short cut geometry pases and assign them the FP. The other two full screen passes
            //! are generic full screen passes and don't need any interaction with the FP.
            bool HairFeatureProcessor::InitShortCutRenderPasses()
            {
                m_hairShortCutGeometryDepthAlphaPass = nullptr;
                m_hairShortCutGeometryShadingPass = nullptr;

                RPI::PassFilter depthAlphaPassFilter = RPI::PassFilter::CreateWithPassName(HairShortCutGeometryDepthAlphaPassName, m_renderPipeline);
                m_hairShortCutGeometryDepthAlphaPass = static_cast<HairShortCutGeometryDepthAlphaPass*>(RPI::PassSystemInterface::Get()->FindFirstPass(depthAlphaPassFilter));
                if (m_hairShortCutGeometryDepthAlphaPass)
                {
                    m_hairShortCutGeometryDepthAlphaPass->SetFeatureProcessor(this);
                }
                else
                {
                    AZ_Error("Hair Gem", false, "HairShortCutResolveDepthPass cannot be found. Check your game project's .pass assets.");
                    return false;
                }

                RPI::PassFilter shaderingPassFilter = RPI::PassFilter::CreateWithPassName(HairShortCutGeometryShadingPassName, m_renderPipeline);
                m_hairShortCutGeometryShadingPass = static_cast<HairShortCutGeometryShadingPass*>(RPI::PassSystemInterface::Get()->FindFirstPass(shaderingPassFilter));
                if (m_hairShortCutGeometryShadingPass)
                {
                    m_hairShortCutGeometryShadingPass->SetFeatureProcessor(this);
                }
                else
                {
                    AZ_Error("Hair Gem", false, "HairShortCutGeometryShadingPass cannot be found. Check your game project's .pass assets.");
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

                // Schedule Srgs binding and the DrawItem build.
                // Since this does not bind the PerPass srg but prepare the rest of the Srgs
                // such as the dynamic srg, it should only be done once per object per frame.
                if (m_usePPLLRenderTechnique)
                {
                    m_hairPPLLRasterPass->SchedulePacketBuild(renderObjectPtr);
                }
                else
                {  
                    m_hairShortCutGeometryDepthAlphaPass->SchedulePacketBuild(renderObjectPtr);
                    m_hairShortCutGeometryShadingPass->SchedulePacketBuild(renderObjectPtr);
                }
            }

            Data::Instance<HairSkinningComputePass> HairFeatureProcessor::GetHairSkinningComputegPass()
            {
                if (!m_computePasses[GlobalShapeConstraintsPassName])
                {
                    Init(m_renderPipeline);
                }
                return m_computePasses[GlobalShapeConstraintsPassName];
            }

            Data::Instance<RPI::Shader> HairFeatureProcessor::GetGeometryRasterShader()
            {
                if (m_usePPLLRenderTechnique)
                {
                    if (!m_hairPPLLRasterPass && !Init(m_renderPipeline))
                    {
                        AZ_Error("Hair Gem", false,
                            "GetGeometryRasterShader - m_hairPPLLRasterPass was not created");
                        return nullptr;
                    }
                    return m_hairPPLLRasterPass->GetShader();
                }

                if (!m_hairShortCutGeometryDepthAlphaPass && !Init(m_renderPipeline))
                {
                    AZ_Error("Hair Gem", false,
                        "GetGeometryRasterShader - m_hairShortCutGeometryDepthAlphaPass was not created");
                    return nullptr;
                }
                return m_hairShortCutGeometryDepthAlphaPass->GetShader();
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ

