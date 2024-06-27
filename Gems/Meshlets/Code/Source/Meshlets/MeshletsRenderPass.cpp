/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

        bool MeshletsRenderPass::FillDrawRequestData(RHI::DeviceDrawPacketBuilder::DeviceDrawRequest& drawRequest)
        {
            if (!m_pipelineState)
            {
                return false;
            }

            drawRequest.m_listTag = m_drawListTag;
            drawRequest.m_pipelineState = m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();

            return true;
        }

        // Adding draw packets
        bool MeshletsRenderPass::AddDrawPackets(AZStd::list<const RHI::DeviceDrawPacket*> drawPackets)
        {
            bool overallSuccess = true;

            if (!m_currentView &&
                (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag)))
            {
                m_currentView = nullptr;    // set it to nullptr to prevent further attempts this frame
                AZ_Warning("Meshlets", false, "AddDrawPackets: failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                return false;
            }
            
            for (const RHI::DeviceDrawPacket* drawPacket : drawPackets)
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
