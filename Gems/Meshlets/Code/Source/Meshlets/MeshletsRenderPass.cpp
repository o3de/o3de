/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

#include <MeshletsRenderPass.h>
#include <MeshletsUtilities.h>
#include <MeshletsFeatureProcessor.h>

namespace AZ
{
    namespace Meshlets
    {
        // --- Creation & Initialization ---
        RPI::Ptr<MeshletsRenderPass> MeshletsRenderPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MeshletsRenderPass> pass = aznew MeshletsRenderPass(descriptor);
            return pass;
        }

        MeshletsRenderPass::MeshletsRenderPass(const RPI::PassDescriptor& descriptor)
            : RasterPass(descriptor),
            m_passDescriptor(descriptor)
        {
            // For inherited classes, override this method and set the proper path.
            // Example: "Shaders/MeshletsDebugRenderShader.azshader"
            SetShaderPath("Shaders/meshletsdebugrendershader.azshader");
            LoadShader();
        }

        bool MeshletsRenderPass::AcquireFeatureProcessor()
        {
            if (m_featureProcessor)
            {
                return true;
            }

            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }

            m_featureProcessor = scene->GetFeatureProcessor<MeshletsFeatureProcessor>();
            if (!m_featureProcessor)
            {
                AZ_Warning("Meshlets", false,
                    "MeshletsRenderPass [%s] - Failed to retrieve Hair feature processor from the scene",
                    GetName().GetCStr());
                return false;
            }
            return true;
        }

        void MeshletsRenderPass::InitializeInternal()
        {
            if (GetScene())
            {
                RasterPass::InitializeInternal();
            }
        }


        bool MeshletsRenderPass::LoadShader()
        {
            RPI::ShaderReloadNotificationBus::Handler::BusDisconnect();

            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(m_passDescriptor);

            // If we successfully retrieved our custom data, use it to set the DrawListTag
            if (!passData)
            {
                AZ_Error("Meshlets", false, "Missing pass raster data");
                return false;
            }

            // Load Shader
            const char* shaderFilePath = m_shaderPath.c_str();
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(shaderFilePath, RPI::AssetUtils::TraceLevel::Error);

            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Error("Meshlets", false, "Invalid shader asset for shader '%s'!", shaderFilePath);
                return false;
            }

            m_shader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_shader)
            {
                AZ_Error("Meshlets", false, "Pass failed to load shader '%s'!", shaderFilePath);
                return false;
            }

            // Per Pass Srg
            {
                // Using 'PerPass' naming since currently RasterPass assumes that the pass Srg is always named 'PassSrg'
                // [To Do] - RasterPass should use srg slot index and not name - currently this will
                //  result in a crash in one of the Atom existing MSAA passes that requires further dive. 
                // m_shaderResourceGroup = UtilityClass::CreateShaderResourceGroup(m_shader, "HairPerPassSrg", "Meshlets");
                m_shaderResourceGroup = UtilityClass::CreateShaderResourceGroup(m_shader, "PassSrg", "Meshlets");
                if (!m_shaderResourceGroup)
                {
                    AZ_Error("Meshlets", false, "Failed to create the per pass srg");
                    return false;
                }
            }
            RPI::ShaderReloadNotificationBus::Handler::BusConnect(shaderAsset.GetId());

            return true;
        }

        bool MeshletsRenderPass::InitializePipelineState()
        {
            if (!m_shader)
            {
                return false;
            }

            const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                AZ_Error("Meshlets", false, "Scene could not be acquired");
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
                AZ_Error("Meshlets", false, "Pipeline state could not be acquired");
                return false;
            }

            return true;
        }

        Data::Instance<RPI::Shader> MeshletsRenderPass::GetShader()
        {
            if (!m_shader)
            {
                AZ_Error("Meshlets", LoadShader(), "MeshletsRenderPass could not initialize pipeline or shader");
            }
            return m_shader;
        }


        //==============================================================================
        // [To Do] - to use the object Id and VertexUtility the ObjectSrg must be created 
        // and passed with the Object Id here.
        // This will require more than several changes also on the CPU side, utilizing the 
        // transform (m_transformService->ReserveObjectId()) and setting the Id in the 
        // PerObjectSrg (objectSrg->SetConstant(objectIdIndex, m_objectId.GetIndex())).
        // Look at ModelDataInstance::Init(Data::Instance<RPI::Model> model) to see how
        // to set the model Id - this will lead to MeshDrawPacket::DoUpdate that will
        // demonstrate how to set the Srgs.
        // The buffers for the render are passed and set via Srg and not as assembly buffers
        // which means that the instance Is must be set (for matrices) and vertex Id must
        // be used in the shader to address the buffers

        // This method is called the first time that a render object is constructed and
        // does not need to be called again.
        // At each frame the MeshletsFeatureProcessor will call the AddDrawPackets per each
        // visible (multi meshlet) mesh and add its draw packets to the view.
        // Notice that because the object Id is mapped 1:1 with the DrawPacket, it currently
        // does not support instancing. For instancing a separate structure per call needs
        // to be added and the DrawPacket as well as the dispatch should be part of an object
        // instance structure and not the render object.
        bool MeshletsRenderPass::BuildDrawPacket(
            ModelLodDataArray& lodRenderDataArray,
            Render::TransformServiceFeatureProcessorInterface::ObjectId objectId)
        {
            for (uint32_t lod = 0; lod < lodRenderDataArray.size(); ++lod)
            {
                MeshRenderData* lodRenderData = lodRenderDataArray[lod];
                if (!lodRenderData || !lodRenderData->RenderObjectSrg)
                {
                    AZ_Error("Meshlets", false,
                        "Failed to build draw packet - missing LOD[%d] render data Render Srg.", lod);
                    return false;
                }

                // ObjectId belongs to the instance and not the object - to be moved
                lodRenderData->ObjectId = objectId;

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = m_drawListTag;
                drawRequest.m_pipelineState = m_pipelineState;
                // Leave the following empty if using buffers rather than vertex streams.
//                drawRequest.m_streamBufferViews = lodRenderData->m_renderStreamBuffersViews; 
                drawRequest.m_stencilRef = 0;
                drawRequest.m_sortKey = 0;

                RHI::DrawPacketBuilder drawPacketBuilder;
                RHI::DrawIndexed drawIndexed;

                drawIndexed.m_indexCount = lodRenderData->IndexCount;
                drawIndexed.m_indexOffset = 0;
                drawIndexed.m_vertexOffset = 0;

                drawPacketBuilder.Begin(nullptr);
                drawPacketBuilder.SetDrawArguments(drawIndexed);
                drawPacketBuilder.SetIndexBufferView(lodRenderData->IndexBufferView);

                // Add the object Id to the Srg - once instancing is supported, the ObjectId and the
                // render Srg should be per instance / draw and not per object.
                RHI::ShaderInputConstantIndex indexConstHandle = lodRenderData->RenderObjectSrg->FindShaderInputConstantIndex(Name("m_objectId"));
                if (!lodRenderData->RenderObjectSrg->SetConstant(indexConstHandle, objectId.GetIndex()))
                {
                    AZ_Error("Meshlets", false, "Failed to bind Render Constant [m_ObjectId]");
                }
                lodRenderData->RenderObjectSrg->Compile();

                // Add the per object render Srg - buffers required for the geometry render
                drawPacketBuilder.AddShaderResourceGroup(lodRenderData->RenderObjectSrg->GetRHIShaderResourceGroup());

                drawPacketBuilder.AddDrawItem(drawRequest);

                // Change the following line in order to support instancing.
                // For instancing the data cannot be associated 1:1 with the object
                lodRenderData->MeshDrawPacket = drawPacketBuilder.End();

                if (!lodRenderData->MeshDrawPacket)
                {
                    AZ_Error("Meshlets", false, "Failed to build the Meshlet DrawPacket.");
                    return false;
                }
            }

            return true;
        }

        // Adding draw packets
        bool MeshletsRenderPass::AddDrawPackets(AZStd::list<const RHI::DrawPacket*> drawPackets)
        {
            bool overallSuccess = true;

            if (!m_currentView &&
                (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag)))
            {
                m_currentView = nullptr;    // set it to nullptr to prevent further attempts this frame
                AZ_Warning("Meshlets", false, "AddDrawPackets: failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                return false;
            }
            
            for (const RHI::DrawPacket* drawPacket : drawPackets)
            {
                if (!drawPacket)
                {   // might not be an error - the object might have just been added and the DrawPacket is
                    // scheduled to be built when the render frame begins
                    AZ_Warning("Meshlets", false, "MeshletsRenderPass - DrawPacket wasn't built");
                    overallSuccess = false;
                    continue;   // other draw packets might be ok - don't break
                }
                m_currentView->AddDrawPacket(drawPacket);
            }
            return overallSuccess;
        }

        void MeshletsRenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_shader && AcquireFeatureProcessor())
            {
                LoadShader();
            }

            if (m_shader && !m_pipelineState)
            {
                InitializePipelineState();
            }

            if (!m_shader || !m_pipelineState)
            {
                return;
            }

            // Refresh current view every frame
            if (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag))
            {
                m_currentView = nullptr;    // set it to null if view exists but no tag match
                AZ_Warning("Meshlets", false, "FrameBeginInternal: failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                return;
            }

            RPI::RasterPass::FrameBeginInternal(params);
        }

        void MeshletsRenderPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (!m_featureProcessor)
            {
                return;
            }

            // Compilation of remaining srgs will be done by the parent class 
            RPI::RasterPass::CompileResources(context);
        }

        void MeshletsRenderPass::BuildShaderAndRenderData()
        {
            m_shader = nullptr; 
            m_pipelineState = nullptr;
            if (!AcquireFeatureProcessor() || !LoadShader() || !InitializePipelineState())
            {
                AZ_Error( "Meshlets", false, "MeshletsRenderPass::BuildShaderAndRenderData failed")
            }
        }

        void MeshletsRenderPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader & shader)
        {
            BuildShaderAndRenderData();
        }

        void MeshletsRenderPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            BuildShaderAndRenderData();
        }

        void MeshletsRenderPass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
        {
            BuildShaderAndRenderData();
        }
    } // namespace Meshlets
}   // namespace AZ
