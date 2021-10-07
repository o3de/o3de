/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridRelocationPass.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridRelocationPass> DiffuseProbeGridRelocationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridRelocationPass> pass = aznew DiffuseProbeGridRelocationPass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridRelocationPass::DiffuseProbeGridRelocationPass(const RPI::PassDescriptor& descriptor)
            : RPI::RenderPass(descriptor)
        {
            LoadShader();
        }

        void DiffuseProbeGridRelocationPass::LoadShader()
        {
            // load shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRelocation.azshader";
            m_shader = RPI::LoadCriticalShader(shaderFilePath);
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
            m_srgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeRelocationPass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        void DiffuseProbeGridRelocationPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().empty())
            {
                // no diffuse probe grids
                return;
            }

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "DiffuseProbeGridRelocationPass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // empty scene
                return;
            }

            // create the Relocation Srgs for each DiffuseProbeGrid, and check to see if any grids need relocation
            bool needRelocation = false;
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                uint32_t rayTracingDataRevision = rayTracingFeatureProcessor->GetRevision();
                if (rayTracingDataRevision != m_rayTracingDataRevision)
                {
                    // the TLAS changed, relocate probes
                    m_rayTracingDataRevision = rayTracingDataRevision;
                    diffuseProbeGrid->ResetRemainingRelocationIterations();
                }

                if (diffuseProbeGrid->GetRemainingRelocationIterations() > 0)
                {
                    needRelocation = true;
                }
            }

            if (!needRelocation)
            {
                // no diffuseProbeGrids require relocation, this pass can be skipped entirely
                return;
            }

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridRelocationPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // probe raytrace image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRayTraceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRayTraceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe relocation image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRelocationImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRelocationImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }
            }
        }

        void DiffuseProbeGridRelocationPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateRelocationSrg(m_shader, m_srgLayout);

                diffuseProbeGrid->GetRelocationSrg()->Compile();

                // relocation stops after a limited number of iterations
                diffuseProbeGrid->DecrementRemainingRelocationIterations();
            }
        }

        void DiffuseProbeGridRelocationPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetRelocationSrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                uint32_t probeCountX;
                uint32_t probeCountY;
                diffuseProbeGrid->GetTexture2DProbeCount(probeCountX, probeCountY);

                RHI::DispatchItem dispatchItem;
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_pipelineState = m_pipelineState;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem);
            }
        }
    }   // namespace Render
}   // namespace AZ
