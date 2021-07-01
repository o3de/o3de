/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridBlendIrradiancePass.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridBlendIrradiancePass> DiffuseProbeGridBlendIrradiancePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridBlendIrradiancePass> pass = aznew DiffuseProbeGridBlendIrradiancePass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridBlendIrradiancePass::DiffuseProbeGridBlendIrradiancePass(const RPI::PassDescriptor& descriptor)
            : RPI::RenderPass(descriptor)
        {
            LoadShader();
        }

        void DiffuseProbeGridBlendIrradiancePass::LoadShader()
        {
            // load shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridBlendIrradiance.azshader";
            m_shader = RPI::LoadShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            // load pipeline state
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);

            // load Pass Srg asset
            m_srgAsset = m_shader->FindShaderResourceGroupAsset(RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto numThreads = m_shader->GetAsset()->GetAttribute(RHI::ShaderStage::Compute, Name{ "numthreads" });
            if (numThreads)
            {
                const RHI::ShaderStageAttributeArguments& args = *numThreads;
                bool validArgs = args.size() == 3;
                if (validArgs)
                {
                    validArgs &= args[0].type() == azrtti_typeid<int>();
                    validArgs &= args[1].type() == azrtti_typeid<int>();
                    validArgs &= args[2].type() == azrtti_typeid<int>();
                }

                if (!validArgs)
                {
                    AZ_Error("PassSystem", false, "[DiffuseProbeBlendIrradiancePass '%s']: Shader '%s' contains invalid numthreads arguments.", GetPathName().GetCStr(), shaderFilePath.c_str());
                    return;
                }

                m_dispatchArgs.m_threadsPerGroupX = AZStd::any_cast<int>(args[0]);
                m_dispatchArgs.m_threadsPerGroupY = AZStd::any_cast<int>(args[1]);
                m_dispatchArgs.m_threadsPerGroupZ = AZStd::any_cast<int>(args[2]);
            }
        }

        void DiffuseProbeGridBlendIrradiancePass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids().empty())
            {
                // no diffuse probe grids
                return;
            }

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "DiffuseProbeGridBlendIrradiancePass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // empty scene
                return;
            }

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridBlendIrradiancePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids())
            {
                // probe raytrace image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRayTraceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRayTraceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe irradiance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe classification image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetClassificationImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeClassificationImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }
            }
        }

        void DiffuseProbeGridBlendIrradiancePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateBlendIrradianceSrg(m_srgAsset);

                diffuseProbeGrid->GetBlendIrradianceSrg()->Compile();
            }
        }

        void DiffuseProbeGridBlendIrradiancePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItem for each DiffuseProbeGrid
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids())
            {
                const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetBlendIrradianceSrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                uint32_t probeCountX;
                uint32_t probeCountY;
                diffuseProbeGrid->GetTexture2DProbeCount(probeCountX, probeCountY);

                RHI::DispatchItem dispatchItem;
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_pipelineState = m_pipelineState;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX * dispatchItem.m_arguments.m_direct.m_threadsPerGroupX;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY * dispatchItem.m_arguments.m_direct.m_threadsPerGroupY;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem);
            }
        }
    }   // namespace Render
}   // namespace AZ
