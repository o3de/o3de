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

//#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>

#include <Passes/HairPPLLRasterPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

#pragma optimize("", off)
namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            // --- Creation & Initialization ---
            RPI::Ptr<HairPPLLRasterPass> HairPPLLRasterPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairPPLLRasterPass> pass = aznew HairPPLLRasterPass(descriptor);
                return pass;
            }

            HairPPLLRasterPass::HairPPLLRasterPass(const RPI::PassDescriptor& descriptor)
                : RasterPass(descriptor),
                m_passDescriptor(descriptor)
            {
            }

            HairPPLLRasterPass::~HairPPLLRasterPass()
            {
            }

            // [To Do] Adi: remove the following two methods since they are no longer required - test with RHI validation
            void HairPPLLRasterPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
            {
                RasterPass::SetupFrameGraphDependencies(frameGraph);
            }

            void HairPPLLRasterPass::OnBuildAttachmentsFinishedInternal()
            {
                RasterPass::OnBuildAttachmentsFinishedInternal();
            }

            void HairPPLLRasterPass::BuildAttachmentsInternal()
            {
                RasterPass::BuildAttachmentsInternal();

                if (!m_featureProcessor)
                {
                    RPI::Scene* scene = GetScene();
                    if (scene)
                    {
                        m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                    }

                    if (!m_featureProcessor)
                    {
                        AZ_Error("Hair Gem", false, "HairPPLLRasterPass - Failed to retrieve Hair feature processor from the scene");
                        return;
                    }
                }

                LoadShaderAndPipelineState();

                if (!m_featureProcessor || !m_shaderResourceGroup)
                {
                    return;
                }

                // Input
                // This is the buffer that is shared between all objects and dispatches and contains
                // the dynamic data that can change between passes.
                // NO need to define it - this is already defined by the Compute pass
//                AttachBufferToSlot(Name{ "SkinnedHairSharedBuffer" }, m_featureProcessor->GetSharedBuffer());

                // Output
                AttachBufferToSlot(Name{ "PPLLIndexCounter" }, m_featureProcessor->GetPerPixelCounterBuffer());
                AttachBufferToSlot(Name{ "PerPixelLinkedList" }, m_featureProcessor->GetPerPixelListBuffer());
            }

            bool HairPPLLRasterPass::IsEnabled() const
            {
                return (RPI::RasterPass::IsEnabled() && m_initialized) ? true : false;
            }

            bool HairPPLLRasterPass::LoadShaderAndPipelineState()
            {
                RPI::ShaderReloadNotificationBus::Handler::BusDisconnect();

                const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(m_passDescriptor);

                // If we successfully retrieved our custom data, use it to set the DrawListTag
                if (!passData)
                {
                    AZ_Error("Hair Gem", false, "Missing pass raster data");
                    return false;
                }

                // Load Shader
                const char* shaderFilePath = "Shaders/hairrenderingfillppll.azshader";
                Data::Asset<RPI::ShaderAsset> shaderAsset =
                    RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(shaderFilePath, RPI::AssetUtils::TraceLevel::Error);

                if (!shaderAsset.GetId().IsValid())
                {
                    AZ_Error("Hair Gem", false, "Invalid shader asset for shader '%s'!", shaderFilePath);
                    return false;
                }

                m_shader = RPI::Shader::FindOrCreate(shaderAsset);
                if (m_shader == nullptr)
                {
                    AZ_Error("Hair Gem", false, "Pass failed to load shader '%s'!", shaderFilePath);
                    return false;
                }

                // Per Pass Srg
                {
                    // Need to use 'PerPass' naming as currently RasterPass assume working with this name!
                    // [To Do] - RasterPass should use srg slot index and not name - currently this will
                    //  result in a crash in one of the Atom existing MSAA passes that requires further dive. 
//                    m_shaderResourceGroup = UtilityClass::CreateShaderResourceGroup(m_shader, "HairPerPassSrg", "Hair Gem");
                    m_shaderResourceGroup = UtilityClass::CreateShaderResourceGroup(m_shader, "PassSrg", "Hair Gem");
                    if (!m_shaderResourceGroup)
                    {
                        AZ_Error("Hair Gem", false, "Failed to create the per pass srg");
                        return false;
                    }
                }

                const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

                RHI::DrawListTag drawListTag = m_shader->GetDrawListTag();
                RPI::Scene* scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
                if (!scene)
                {
                    AZ_Error("Hair Gem", false, "Scene could not be acquired" );
                    return false;
                }
                scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                pipelineStateDescriptor.m_inputStreamLayout.SetTopology(AZ::RHI::PrimitiveTopology::TriangleList);
                pipelineStateDescriptor.m_inputStreamLayout.Finalize();

                m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
                if (!m_pipelineState)
                {
                    AZ_Error("Hair Gem", false, "Pipeline state could not be acquired");
                    return false;
                }

                RPI::ShaderReloadNotificationBus::Handler::BusConnect(shaderAsset.GetId());

                m_initialized = true;
                return true;
            }

            bool HairPPLLRasterPass::BuildDrawPacket(HairRenderObject* hairObject)
            {
                if (!m_shader || !m_pipelineState || !m_shaderResourceGroup)
                {
                    return false;
                }

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = m_drawListTag;
                drawRequest.m_pipelineState = m_pipelineState;
//                drawRequest.m_streamBufferViews =  // nor explicit vertex buffer.  shader is using the srg buffers
                drawRequest.m_stencilRef = 0;
                drawRequest.m_sortKey = 0;

                // Seems that the PerView and PerScene are gathered through RenderPass::CollectSrgs()
                // The PerPass is gathered through the RasterPass::m_shaderResourceGroup
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                return hairObject->BuildPPLLDrawPacket(drawRequest);
            }


            bool HairPPLLRasterPass::AddDrawPacket(HairRenderObject* hairObject)
            {
                const RPI::ViewPtr currentView = GetView();
                if (!currentView)
                {
                    return false;
                }

                // If this view is ignoring packets with our draw list tag then skip this view
                if (!currentView->HasDrawListTag(m_drawListTag))
                {
                    AZ_Warning("Hair Gem", false, "HairPPLLRasterPass failed to match the DrawListTag - check that your pass and shader tag name match");
                    return false;
                }

                const RHI::DrawPacket* drawPacket = hairObject->GetFillDrawPacket();
                if (!drawPacket)
                {
                    AZ_Error("Hair Gem", false, "HairPPLLRasterPass failed to get or create the DrawPacket");
                    return false;
                }

                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                currentView->AddDrawPacket(drawPacket);

                return true;
            }

            void HairPPLLRasterPass::FrameBeginInternal(FramePrepareParams params)
            {
                {
                    if (auto ppllCounterBuffer = m_featureProcessor->GetPerPixelCounterBuffer())
                    {   // This is crucial to be able to count properly from 0. Currently trying to
                        // do this from the pass does not work.
                        // [To Do] Adi: try to move the transient creation to be pass data driven.
                        uint32_t sourceData = 0;
                        bool updateSuccess = ppllCounterBuffer->UpdateData(&sourceData, sizeof(uint32_t), 0);
                        AZ_Warning("Hair Gem", updateSuccess, "HairPPLLRasterPass::CompileResources could not reset PPLL counter");
                    }
                }

                RPI::RasterPass::FrameBeginInternal(params);
            }

            // [To Do] Adi: since it no longer have special purpose, remove and test
            void HairPPLLRasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);

                if (!m_featureProcessor)
                {
                    return;
                }

                // Compilation of remaining srgs will be done by the parent class 
                RPI::RasterPass::CompileResources(context);
            }

            void HairPPLLRasterPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader & shader)
            {
                LoadShaderAndPipelineState();
                m_featureProcessor->ForceRebuildRenderData();
            }

            void HairPPLLRasterPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
            {
                LoadShaderAndPipelineState();
                m_featureProcessor->ForceRebuildRenderData();
            }

            void HairPPLLRasterPass::OnShaderVariantReinitialized([[maybe_unused]] const RPI::Shader& shader, [[maybe_unused]] const RPI::ShaderVariantId& shaderVariantId, [[maybe_unused]] RPI::ShaderVariantStableId shaderVariantStableId)
            {
                LoadShaderAndPipelineState();
                m_featureProcessor->ForceRebuildRenderData();
            }
        } // namespace Hair
    }   // namespace Render
}   // namespace AZ

#pragma optimize("", on)
