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

            HairFeatureProcessor::HairFeatureProcessor()
            {
            }

            void HairFeatureProcessor::Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext
                        ->Class<HairFeatureProcessor, RPI::FeatureProcessor>()
                        ->Version(0);
                }
            }

            void HairFeatureProcessor::Activate()
            {
                // Adding all Hair passes
//               auto* passSystem = RPI::PassSystemInterface::Get();
//                AZ_Assert(passSystem, "Cannot get the pass system.");
//                passSystem->AddPassCreator(Name("HairSkinningComputePass"), &HairSkinningComputePass::Create);
//                InitAllPasses();
//                Init();       // the activate is happening before the pass system exists - cannot work.
                // Passes will be activate by the pass system and so no need to init them - just connect on time.

                // Create right away for everyone to use
                if (!CreateAndBindPerPassSrg())
                {   // this might not be an error - if the pass system is still empty / minimal
                    //  and these passes are not part of the minimal pipeline, they will not
                    //  be created.
                    AZ_Error("Hair Gem", false, "Failed to create the PerPass Srg.");
                }

                EnableSceneNotification();
            }

            void HairFeatureProcessor::Deactivate()
            {
                DisableSceneNotification();
                m_sharedDynamicBuffer.reset();
            }

            //==========================================================================================
            //  TressFX Hair Render functionality
            //==========================================================================================
            void HairFeatureProcessor::AddHairRenderObject(Data::Instance<HairRenderObject> renderObject)
            {
                m_hairRenderObjects.push_back(renderObject);
                m_hairPPLLRasterPass->BuildDrawPacket(renderObject.get());
            }

            bool HairFeatureProcessor::RemoveHairRenderObject(Data::Instance<HairRenderObject> renderObject)
            {
                for ( auto objIter = m_hairRenderObjects.begin() ; objIter != m_hairRenderObjects.end() ; ++objIter )
                {
                    if (objIter->get() == renderObject)
                    {
                        m_hairRenderObjects.erase(objIter);
                        return true;
                    }
                }
                return false;
            }

            // Adi: The feature processor might be the one triggering this (or not), but
            // in any case this should be done via the passes and not the feature processor
            // Limited method to update the hair skinning matrices.
            // MOVE MOST OF THIS METHOD TO THE PASS
            // It avoid any physics and simulation response and should be used for initial integration testing
            void HairFeatureProcessor::UpdateHairSkinning()
            {
                // Copying CPU side m_SimCB content to the GPU buffer (matrices, wind parameters..) 

                // Adi: more initialization might be required here if we prep for more than
                // only the skinning pass
                // Adi: this requires to update the matrices data within the buffer as well!!
                for (auto objIter = m_hairRenderObjects.begin(); objIter != m_hairRenderObjects.end(); ++objIter)
                {
                    objIter->get()->UpdateConstantBuffer();
                }

                /*  Adibugbug
                /////////////
                // Adi: the following should be done by the pass that will send each object to be done
                // by K dispatches based on the amount of vertices it has.
                // Adi: only skin hair vertices without any physics. 
                DispatchComputeShader(commandContext, mSkinHairVerticesTestPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
                GetDevice()->GetTimeStamp("SkinHairVerticesTestPSO");
                */

                // UpdateFollowHairVertices - This part is embedded in the single pass shader
                //    DispatchComputeShader(commandContext, mUpdateFollowHairVerticesPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
                //    GetDevice()->GetTimeStamp("UpdateFollowHairVertices");

                // Adi: Advance the frame, is it required when dual buffers are supported explicitly?

                for (auto objIter = m_hairRenderObjects.begin(); objIter != m_hairRenderObjects.end(); ++objIter)
                {
                    objIter->get()->IncreaseSimulationFrame();
                }
            }

            //! Assumption: the hair is being updated per object before this method is called and
            //!  therefore the parameters that were calculated per object can be directly copied
            //!  without need to recalculate as in the original code.
            //! Make sure there are no more than (currently) 16 hair objects or update dynamic handling.
            //! Originally called 'UpdateShadeParameters' in TressFX
            void HairFeatureProcessor::CreateHairMaterialsArray(std::vector<const AMD::TressFXRenderParams*>& renderSettings)
            {
                // [To Do] Adi: this needs to be initialized when creating the render passes
                //                bool bindSuccess = m_hairObjectsMaterialsCB.CreateAndSetBindIndex(m_hairRenderSrg,
                //                    m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::RenderCB)]);

                // Update Render Parameters
                for (int i = 0; i < renderSettings.size(); ++i)
                {
                    m_hairObjectsMaterialsCB->HairShadeParams[i].FiberRadius = renderSettings[i]->FiberRadius;
                    m_hairObjectsMaterialsCB->HairShadeParams[i].ShadowAlpha = renderSettings[i]->ShadowAlpha;
                    m_hairObjectsMaterialsCB->HairShadeParams[i].FiberSpacing = renderSettings[i]->FiberSpacing;
                    m_hairObjectsMaterialsCB->HairShadeParams[i].HairEx2 = renderSettings[i]->HairEx2;
                    m_hairObjectsMaterialsCB->HairShadeParams[i].HairKs2 = renderSettings[i]->HairKs2;
                    m_hairObjectsMaterialsCB->HairShadeParams[i].MatKValue = renderSettings[i]->MatKValue;
                }

                // Update the constant buffer that is part of the per pass srg
                m_hairObjectsMaterialsCB.UpdateGPUData();  
            }

            //==========================================================================================
            void HairFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);
                AZ_ATOM_PROFILE_FUNCTION("Hair", "HairFeatureProcessor: Simulate");
                AZ_UNUSED(packet);

//                if (!m_initialized && !m_hairRenderObjects.empty())
//                {
//                    Init();
//                }
//
                if (!m_initialized)
                {   // there are no render objects yet, indicating that scene data might not be ready
                    // to initialize just yet.
                    return;
                }

                // Skinning matrices update and set in buffer
                if (m_hairSkinningComputePass)// || InitSkinningPass())
                {
                    UpdateHairSkinning();
                }

                // Prepare materials array for the per pass srg
                std::vector<const AMD::TressFXRenderParams*> hairObjectsRenderMaterials;
                uint32_t obj = 0;
                for ( auto objIter = m_hairRenderObjects.begin() ; objIter != m_hairRenderObjects.end(); ++objIter, ++obj )
                {
                    HairRenderObject* renderObject = objIter->get();
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
                CreateHairMaterialsArray(hairObjectsRenderMaterials);

//            HairFeatureProcessorNotificationBus::Broadcast(&HairFeatureProcessorNotificationBus::Events::OnUpdateSkinningMatrices);
            }


            void HairFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);
                AZ_ATOM_PROFILE_FUNCTION("Hair", "HairFeatureProcessor: Render");

                if (!m_initialized)
                {
                    return;
                }

//                m_perPassSrg->Compile();

                // Adi: should this be moved to 'OnBeginPrepareRender' / 'SImulate'?
                // [To Do] Adi: missing culling scheme - setup the hair culling work group
                // (to be re-used for each view). See SkinnedMeshFeatureProcessor::Render for more details
                for (auto objIter = m_hairRenderObjects.begin(); objIter != m_hairRenderObjects.end(); ++objIter)
                {
                    HairRenderObject* renderObject = objIter->get();
                    m_hairSkinningComputePass->AddDispatchItem(renderObject->GetSkinningDispatchItem().GetDispatchItem());
                    m_hairPPLLRasterPass->AddDrawPacket(renderObject);
                }
            }
    
            void HairFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr pipeline)
            {
                Init();
            }

            void HairFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
            {
//                Init();
            }

            void HairFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
            {
//                Init();
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
                if (m_initialized)
                {
                    return true;
                }

                bool resultSuccess = InitSkinningPass();
                resultSuccess &= InitPPLLFillPass();
                resultSuccess &= InitPPLLResolvePass();

                if (resultSuccess && !m_perPassSrg)
                {   // this might not be an error - if the pass system is still empty / minimal
                    //  and these passes are not part of the minimal pipeline, they will not
                    //  be created.
                    AZ_Error("Hair Gem", false, "Passes were not retrieved or PerPass Srg was not created.");
                    return false;
                }

                if (resultSuccess == true)
                {
                    m_hairSkinningComputePass->SetFeatureProcessor(this);
                    m_hairPPLLRasterPass->SetFeatureProcessor(this);
//                    m_hairPPLLRasterPass->Init();
                    m_hairPPLLResolvePass->SetFeatureProcessor(this);
                    m_initialized = true;
                }

                return m_initialized;
            }

            bool HairFeatureProcessor::GetHairSkinningComputeShader()
            {
                const char* shaderFilePath = "Shaders/hairskinningcompute.azshader";
                Data::Asset<RPI::ShaderAsset> shaderAsset =
                    RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(shaderFilePath, RPI::AssetUtils::TraceLevel::Error);

                m_hairSkinningComputerShader = RPI::Shader::FindOrCreate(shaderAsset);
                if (m_hairSkinningComputerShader == nullptr)
                {
                    AZ_Error("Hair Gem", false, "Feature processor failed to load shader '%s'!", shaderFilePath);
                    return false;
                }
                return true;
            }

            bool HairFeatureProcessor::CreateAndBindPerPassSrg()
            {
                if (m_passSrgCreated)
                {
                    return true;
                }

                if (!GetHairSkinningComputeShader())
                {
                    AZ_Error("Hair Gem", false, "Feature Processor Error: shader is not initialized - can't create Per Pass Srg");
                    return false;
                }

                // Per Pass Srg
                {
                    m_perPassSrg = UtilityClass::CreateShaderResourceGroup(m_hairSkinningComputerShader, "HairPerPassSrg", "Hair Gem");
                    if (!m_perPassSrg)
                    {
                        AZ_Error("Hair Gem", false, "Failed to create the per pass srg");
                        return false;
                    }
                }

                SrgBufferDescriptor descriptor;

                // Material array constant buffer
                {
                    descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                        sizeof(AMD::TressFXShadeParams), 1,
                        Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                    );

                    if (!m_hairObjectsMaterialsCB.CreateAndBindToSrg(m_perPassSrg, descriptor))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind material array constant buffer [%s]", descriptor.m_bufferName.GetCStr());
                        return false;
                    }
                }

                // Shared buffer 
                {
                    //-------------------------------------------------------------
                    // [To Do] Adi: enhance interface to support many shared buffers that can be acquired and
                    //      retrieved using a string name or a Name.
                    AZStd::vector<SrgBufferDescriptor> hairDynamicDescriptors;
                    DynamicHairData::PrepareSrgDescriptors(hairDynamicDescriptors, 1, 1);
                    m_sharedDynamicBuffer = AZStd::make_unique<SharedBuffer>("HairSharedDynamicBuffer", hairDynamicDescriptors);

                    Name sharedBufferName = Name("m_skinnedHairSharedBuffer");
                    RHI::ShaderInputBufferIndex indexHandle = m_perPassSrg->FindShaderInputBufferIndex(sharedBufferName);
                    if (!m_perPassSrg->SetBufferView(indexHandle, SharedBufferInterface::Get()->GetBuffer()->GetBufferView()))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", sharedBufferName.GetCStr());
                        return false;
                    }
                }

                // PPLL nodes buffer
                {
                    descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                        Name{ "LinkedListNodesPPLL" }, Name{ "m_linkedListNodes" }, 0, 0
                    );
                    m_linkedListNodesBuffer = UtilityClass::CreateBufferAndBindToSrg("PerPassSrg", descriptor, m_perPassSrg);
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
                        Name{ "LinkedListCounterPPLL" }, Name{ "m_linkedListCounter" }, 0, 0
                    );
                    m_linkedListCounterBuffer = UtilityClass::CreateBufferAndBindToSrg("PerPassSrg", descriptor, m_perPassSrg);
                    if (!m_linkedListCounterBuffer)
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", descriptor.m_bufferName.GetCStr());
                        return false;
                    }
                }

                // PPLL image based list headers
                {
//                    RPI::Scene* scene = m_hairSkinningComputePass->GetScene();
//                    AZ_Error("Hair Gem", scene, "Failed to get the scene");                 
//                    AZ::RPI::ViewPtr currentView = scene ? scene->GetDefaultRenderPipeline()->GetDefaultView() : nullptr;
                    // [To Do] Adi: this needs to be connected to the frame viewport dimension and
                    // recreate itself if the size changes.  For now it can be the largest possible
                    // to make sure pixel writes are not out of bound.
                    const RPI::ViewPtr currentView = m_hairSkinningComputePass ? m_hairSkinningComputePass->GetView() : nullptr;
                    uint32_t width = 1920, height = 1080;   // default to 1080p resolution
                    if (currentView)
                    {
                        const AZ::Matrix4x4& viewClipMatric = currentView->GetViewToClipMatrix();
                        width = viewClipMatric.GetElement(0, 0);
                        height = viewClipMatric.GetElement(1, 1);

                        // adibugbug - the above does not bring the projection to screen as desired
                        width = 1920;
                        height = 1080;   // default to 1080p resolution
                    }
                    AZ_Error("Hair Gem", currentView, "Failed to get the pass view");

                    RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                        RHI::ImageBindFlags::ShaderReadWrite, // GPU read and write is allowed
                        width, height, RHI::Format::R32_UINT
                    );

                    if (!CreateAndSetPerPixelHeadImage(imageDesc))
                    {   // Error messaged are in the method itself
                        return false;
                    }
                }

//                m_perPassSrg->Compile();
                m_passSrgCreated = true;
                return true;
            }

            bool HairFeatureProcessor::CreateAndSetPerPixelHeadImage( RHI::ImageDescriptor& imageDesc )
            {
                if (!m_pool)
                {
                    m_pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
                }

                if (!m_pool)
                {
                    AZ_Error("Hair Gem", false, "Failed to initialize image pool for [m_fragmentListHead]");
                    return false;
                }

                RHI::ClearValue clearValue;
                m_linkedListPerPixelHead = RPI::AttachmentImage::Create(
                    *m_pool.get(), imageDesc, Name("m_fragmentListHead"), &clearValue);

                if (!m_linkedListPerPixelHead)
                {
                    AZ_Error("Hair Gem", false, "Failed to initialize PPLL headers image");
                    return false;
                }

                RHI::ShaderInputImageIndex imageIndex = m_perPassSrg->FindShaderInputImageIndex(Name("m_fragmentListHead"));
                if (!m_perPassSrg->SetImageView(imageIndex, m_linkedListPerPixelHead->GetImageView()))
                {
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [m_fragmentListHead]");
                    return false;
                }

                return true;
            }

            // Adi: is this required? if data driven via the MainPipeline.pass the answer is probably no?!
            bool HairFeatureProcessor::InitSkinningPass()
            {
                m_hairSkinningComputePass = nullptr;   // reset it to null, just in case it fails to load the assets properly

                RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();
                if (passSystem->HasPassesForTemplateName(AZ::Name{ "HairSkinningComputePassTemplate" }))
                {
                    auto& desiredPasses = passSystem->GetPassesForTemplateName(AZ::Name{ "HairSkinningComputePassTemplate" });

                    // For now, assume one skinning pass
                    if (!desiredPasses.empty() && desiredPasses[0])
                    {
                        m_hairSkinningComputePass = static_cast<HairSkinningComputePass*>(desiredPasses[0]);
                    }
                    else
                    {
                        AZ_Error("Hair Gem", false, "HairSkinningPassTemplate does not have any valid passes. Check your game project's .pass assets.");
                        return false;
                    }
                }
                else
                {
                    AZ_Error("Hair Gem", false, "Failed to find passes for HairSkinningPassTemplate. Check your game project's .pass assets.");
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
                    }
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

        } // namespace Hair
    } // namespace Render
} // namespace AZ

#pragma optimize("", on)
