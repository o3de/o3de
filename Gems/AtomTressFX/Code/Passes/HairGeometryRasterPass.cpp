/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DeviceDrawPacketBuilder.h>
#include <Atom/RHI/DevicePipelineState.h>

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

#include <Passes/HairGeometryRasterPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            // --- Creation & Initialization ---
            RPI::Ptr<HairGeometryRasterPass> HairGeometryRasterPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairGeometryRasterPass> pass = aznew HairGeometryRasterPass(descriptor);
                return pass;
            }

            HairGeometryRasterPass::HairGeometryRasterPass(const RPI::PassDescriptor& descriptor)
                : RasterPass(descriptor),
                m_passDescriptor(descriptor)
            {
                // For inherited classes, override this method and set the proper path.
                // Example: "Shaders/hairrenderingfillppll.azshader"
                SetShaderPath("dummyShaderPath");
            }

            bool HairGeometryRasterPass::AcquireFeatureProcessor()
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
                        "HairGeometryRasterPass [%s] - Failed to retrieve Hair feature processor from the scene",
                        GetName().GetCStr());
                    return false;
                }
                return true;
            }

            void HairGeometryRasterPass::InitializeInternal()
            {
                if (GetScene())
                {
                    RasterPass::InitializeInternal();
                }
            }

            bool HairGeometryRasterPass::IsEnabled() const
            {
                return (RPI::RasterPass::IsEnabled() && m_initialized) ? true : false;
            }

            bool HairGeometryRasterPass::UpdateShaderOptions(const RPI::ShaderVariantId& variantId)
            {
                m_currentShaderVariantId = variantId;
                const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(m_currentShaderVariantId);
                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, m_currentShaderVariantId);

                RPI::Scene* scene = GetScene();
                if (!scene)
                {
                    AZ_Error("Hair Gem", false, "Scene could not be acquired");
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

                if (m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry() && shaderVariant.UseKeyFallback())
                {
                    m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_currentShaderVariantId.m_key);
                }
                return true;
            }

            bool HairGeometryRasterPass::LoadShaderAndPipelineState()
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
                const char* shaderFilePath = m_shaderPath.c_str();
                Data::Asset<RPI::ShaderAsset> shaderAsset =
                    RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(shaderFilePath, RPI::AssetUtils::TraceLevel::Error);

                if (!shaderAsset.IsReady())
                {
                    AZ_Error("Hair Gem", false, "Invalid shader asset for shader '%s'!", shaderFilePath);
                    return false;
                }

                m_shader = RPI::Shader::FindOrCreate(shaderAsset);
                if (m_shader == nullptr)
                {
                    AZ_Error("Hair Gem", false, "Pass failed to create shader instance from asset '%s'!", shaderFilePath);
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

                if (!UpdateShaderOptions(m_shader->GetDefaultShaderOptions().GetShaderVariantId()))
                {
                    AZ_Error("Hair Gem", false, "Failed to create pipeline state");
                    return false;
                }

                RPI::ShaderReloadNotificationBus::Handler::BusConnect(shaderAsset.GetId());

                m_initialized = true;
                return true;
            }

            Data::Instance<RPI::Shader> HairGeometryRasterPass::GetShader()
            {
                if (!m_initialized || !m_shader)
                {
                    AZ_Error("Hair Gem", LoadShaderAndPipelineState(), "HairGeometryRasterPass could not initialize pipeline or shader");
                }
                return m_shader;
            }

            void HairGeometryRasterPass::SchedulePacketBuild(HairRenderObject* hairObject)
            {
                m_newRenderObjects.insert(hairObject);
                BuildDrawPacket(hairObject);
            }

            bool HairGeometryRasterPass::BuildDrawPacket(HairRenderObject* hairObject)
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

                return hairObject->BuildDrawPacket(m_shader.get(), drawRequest);
            }

            bool HairGeometryRasterPass::AddDrawPackets(AZStd::list<Data::Instance<HairRenderObject>>& hairRenderObjects)
            {
                bool overallSuccess = true;

                if (!m_currentView &&
                    (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag)))
                {
                    m_currentView = nullptr;    // set it to nullptr to prevent further attempts this frame
                    AZ_Warning("Hair Gem", false, "AddDrawPackets: failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                    return false;
                }

                for (auto& renderObject : hairRenderObjects)
                {
                    const RHI::DrawPacket* drawPacket = renderObject->GetGeometrylDrawPacket(m_shader.get());
                    if (!drawPacket)
                    {   // might not be an error - the object might have just been added and the DrawPacket is
                        // scheduled to be built when the render frame begins
                        AZ_Warning("Hair Gem", !m_newRenderObjects.empty(), "HairGeometryRasterPass - DrawPacket wasn't built");
                        overallSuccess = false;
                        continue;
                    }

                    m_currentView->AddDrawPacket(drawPacket);
                }
                return overallSuccess;
            }

            void HairGeometryRasterPass::FrameBeginInternal(FramePrepareParams params)
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
                }

                // Clear the new added objects - BuildDrawPacket should only be carried out once per
                // object/shader lifetime
                m_newRenderObjects.clear();

                // Refresh current view every frame
                if (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag))
                {
                    m_currentView = nullptr;    // set it to null if view exists but no tag match
                    AZ_Warning("Hair Gem", false, "FrameBeginInternal: failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                    return;
                }

                RPI::RasterPass::FrameBeginInternal(params);
            }

            void HairGeometryRasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                AZ_PROFILE_FUNCTION(AzRender);

                if (!m_featureProcessor)
                {
                    return;
                }

                // Compilation of remaining srgs will be done by the parent class 
                RPI::RasterPass::CompileResources(context);
            }

            void HairGeometryRasterPass::BuildShaderAndRenderData()
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_initialized = false;  // make sure we initialize it even if not in this frame
                if (AcquireFeatureProcessor())
                {
                    LoadShaderAndPipelineState();
                    m_featureProcessor->ForceRebuildRenderData();
                }
            }

            void HairGeometryRasterPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader & shader)
            {
                BuildShaderAndRenderData();
            }

            void HairGeometryRasterPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
            {
                BuildShaderAndRenderData();
            }

            void HairGeometryRasterPass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
            {
                BuildShaderAndRenderData();
            }
        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
