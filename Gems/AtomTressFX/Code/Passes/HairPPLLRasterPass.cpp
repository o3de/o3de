/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#include <Passes/HairPPLLRasterPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

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

            bool HairPPLLRasterPass::AcquireFeatureProcessor()
            {
                if (m_featureProcessor)
                {
                    return true;
                }

                RPI::Scene* scene = GetScene();
                if (scene)
                {
                    m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                }
                else
                {
                    return false;
                }

                if (!m_featureProcessor)
                {
                    AZ_Warning("Hair Gem", false,
                        "HairPPLLRasterPass [%s] - Failed to retrieve Hair feature processor from the scene",
                        GetName().GetCStr());
                    return false;
                }
                return true;
            }

            void HairPPLLRasterPass::InitializeInternal()
            {
                if (GetScene())
                {
                    RasterPass::InitializeInternal();
                }
            }

            void HairPPLLRasterPass::BuildInternal()
            {
                RasterPass::BuildInternal();

                if (!AcquireFeatureProcessor())
                {
                    return;
                }

                if (!LoadShaderAndPipelineState())
                {
                    return;
                }

                // Output
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
                    // Using 'PerPass' naming since currently RasterPass assumes that the pass Srg is always named 'PassSrg'
                    // [To Do] - RasterPass should use srg slot index and not name - currently this will
                    //  result in a crash in one of the Atom existing MSAA passes that requires further dive. 
                    // m_shaderResourceGroup = UtilityClass::CreateShaderResourceGroup(m_shader, "HairPerPassSrg", "Hair Gem");
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

                RPI::Scene* scene = GetScene();
                if (!scene)
                {
                    AZ_Error("Hair Gem", false, "Scene could not be acquired" );
                    return false;
                }
                RHI::DrawListTag drawListTag = m_shader->GetDrawListTag();
                scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                pipelineStateDescriptor.m_renderAttachmentConfiguration = GetRenderAttachmentConfiguration();
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

            void HairPPLLRasterPass::SchedulePacketBuild(HairRenderObject* hairObject)
            {
                m_newRenderObjects.insert(hairObject);
            }

            bool HairPPLLRasterPass::BuildDrawPacket(HairRenderObject* hairObject)
            {
                if (!m_initialized)
                {
                    return false;
                }

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = m_drawListTag;
                drawRequest.m_pipelineState = m_pipelineState;
//                drawRequest.m_streamBufferViews =  // no explicit vertex buffer.  shader is using the srg buffers
                drawRequest.m_stencilRef = 0;
                drawRequest.m_sortKey = 0;

                // Seems that the PerView and PerScene are gathered through RenderPass::CollectSrgs()
                // The PerPass is gathered through the RasterPass::m_shaderResourceGroup
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                return hairObject->BuildPPLLDrawPacket(drawRequest);
            }

            bool HairPPLLRasterPass::AddDrawPackets(AZStd::list<Data::Instance<HairRenderObject>>& hairRenderObjects)
            {
                bool overallSuccess = true;

                if (!m_currentView &&
                    (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag)))
                {
                    m_currentView = nullptr;    // set it to nullptr to prevent further attempts this frame
                    AZ_Warning("Hair Gem", false, "HairPPLLRasterPass failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                    return false;
                }

                for (auto& renderObject : hairRenderObjects)
                {
                    const RHI::DrawPacket* drawPacket = renderObject->GetFillDrawPacket();
                    if (!drawPacket)
                    {   // might not be an error - the object might have just been added and the DrawPacket is
                        // scheduled to be built when the render frame begins
                        AZ_Warning("Hair Gem", !m_newRenderObjects.empty(), "HairPPLLRasterPass - DrawPacket wasn't built");
                        overallSuccess = false;
                        continue;
                    }

                    m_currentView->AddDrawPacket(drawPacket);
                }
                return overallSuccess;
            }

            void HairPPLLRasterPass::FrameBeginInternal(FramePrepareParams params)
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                    if (!m_initialized && AcquireFeatureProcessor())
                    {
                        LoadShaderAndPipelineState();
                        m_featureProcessor->ForceRebuildRenderData();
                    }
                }

                if (!m_initialized)
                {
                    return;
                }

                // Bind the Per Object resources and trigger the RHI validation that will use attachment
                // for its validation.  The attachments are invalidated outside the render begin/end frame.
                for (HairRenderObject* newObject : m_newRenderObjects)
                {
                    newObject->BindPerObjectSrgForRaster();
                    BuildDrawPacket(newObject);
                }

                // Clear the new added objects - BuildDrawPacket should only be carried out once per
                // object/shader lifetime
                m_newRenderObjects.clear();

                // Refresh current view every frame
                if (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag))
                {
                    m_currentView = nullptr;    // set it to null if view exists but no tag match
                    AZ_Warning("Hair Gem", false, "HairPPLLRasterPass failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                    return;
                }

                RPI::RasterPass::FrameBeginInternal(params);
            }

            void HairPPLLRasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                AZ_PROFILE_FUNCTION(AzRender);

                if (!m_featureProcessor)
                {
                    return;
                }

                // Compilation of remaining srgs will be done by the parent class 
                RPI::RasterPass::CompileResources(context);
            }

            void HairPPLLRasterPass::BuildShaderAndRenderData()
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_initialized = false;  // make sure we initialize it even if not in this frame
                if (AcquireFeatureProcessor())
                {
                    LoadShaderAndPipelineState();
                    m_featureProcessor->ForceRebuildRenderData();
                }
            }

            void HairPPLLRasterPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader & shader)
            {
                BuildShaderAndRenderData();
            }

            void HairPPLLRasterPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
            {
                BuildShaderAndRenderData();
            }

            void HairPPLLRasterPass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
            {
                BuildShaderAndRenderData();
            }
        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
