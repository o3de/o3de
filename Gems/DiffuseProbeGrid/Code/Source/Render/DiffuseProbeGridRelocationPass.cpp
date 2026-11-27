/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseProbeGridRelocationPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridRelocationPass> DiffuseProbeGridRelocationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridRelocationPass> pass = aznew DiffuseProbeGridRelocationPass(descriptor);
            return pass;
        }

        DiffuseProbeGridRelocationPass::DiffuseProbeGridRelocationPass(const RPI::PassDescriptor& descriptor)
            : RPI::RenderPass(descriptor)
        {
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
            }
            else
            {
                LoadShader(m_relocation, "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRelocation.azshader");
                LoadShader(m_contactRelocation, "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridContactRelocation.azshader");
                if (!m_contactRelocation.m_pipelineState)
                {
                    m_contactRelocation = m_relocation;
                }
            }
        }

        void DiffuseProbeGridRelocationPass::LoadShader(RelocationShader& shader, const char* shaderFilePath)
        {
            // load shader
            // Note: the shader may not be available on all platforms
            shader.m_shader = RPI::LoadCriticalShader(shaderFilePath);
            if (shader.m_shader == nullptr)
            {
                return;
            }

            // load pipeline state
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = shader.m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shader.m_shader->GetDefaultShaderOptions());
            shader.m_pipelineState = shader.m_shader->AcquirePipelineState(pipelineStateDescriptor);

            // load Pass Srg asset
            shader.m_srgLayout = shader.m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = RPI::GetComputeShaderNumThreads(shader.m_shader->GetAsset(), shader.m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                shader.m_pipelineState = nullptr;
                AZ_Error("PassSystem", false, "[DiffuseProbeRelocationPass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath, outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridRelocationPass::IsEnabled() const
        {
            if (!m_relocation.m_pipelineState || !RenderPass::IsEnabled())
            {
                return false;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this).empty())
            {
                // no diffuse probe grids
                return false;
            }

            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessorInterface>();
            // check TLAS version
            uint32_t rayTracingDataRevision = !diffuseProbeGridFeatureProcessor->GetVisibleHardwareProbeGrids().empty() && rayTracingFeatureProcessor ?
                rayTracingFeatureProcessor->GetRevision() : m_rayTracingDataRevision;
            if (rayTracingDataRevision != m_rayTracingDataRevision)
            {
                return true;
            }

            // check to see if any grids need relocation
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this))
            {
                if (diffuseProbeGrid->GetRemainingRelocationIterations() > 0)
                {
                    return true;
                }
            }

            return false;
        }

        void DiffuseProbeGridRelocationPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessorInterface>();

            // reset the relocation iterations on the grids if the TLAS was updated
            uint32_t rayTracingDataRevision = !diffuseProbeGridFeatureProcessor->GetVisibleHardwareProbeGrids().empty() && rayTracingFeatureProcessor ?
                rayTracingFeatureProcessor->GetRevision() : m_rayTracingDataRevision;
            if (rayTracingDataRevision != m_rayTracingDataRevision)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleHardwareProbeGrids())
                {
                    diffuseProbeGrid->ResetRemainingRelocationIterations();
                }
            }

            m_rayTracingDataRevision = rayTracingDataRevision;

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridRelocationPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this).size()));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this))
            {
                // grid data
                {
                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe raytrace image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRayTraceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRayTraceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe data image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
                }
            }
        }

        void DiffuseProbeGridRelocationPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this))
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() in ShaderResourceGroupData.cpp)
                if (diffuseProbeGrid->GetRemainingRelocationIterations() == 0)
                {
                    continue;
                }
                const auto& shader = diffuseProbeGrid->IsContactRelocation() ? m_contactRelocation : m_relocation;
                diffuseProbeGrid->UpdateRelocationSrg(shader.m_shader, shader.m_srgLayout);
                const auto& srg = diffuseProbeGrid->GetRelocationSrg();
                if (!srg->IsQueuedForCompile())
                {
                    srg->Compile();
                }
            }
        }

        void DiffuseProbeGridRelocationPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            const auto& grids = diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this);
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                const auto& diffuseProbeGrid = grids[index];
                if (diffuseProbeGrid->GetRemainingRelocationIterations() == 0)
                {
                    continue;
                }

                const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetRelocationSrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

                RHI::DeviceDispatchItem dispatchItem;
                const auto& shader = diffuseProbeGrid->IsContactRelocation() ? m_contactRelocation : m_relocation;
                dispatchItem.m_arguments = shader.m_dispatchArgs;
                dispatchItem.m_pipelineState = shader.m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = AZ::DivideAndRoundUp(diffuseProbeGrid->GetTotalProbeCount(), diffuseProbeGrid->GetFrameUpdateCount());
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 1;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }

        void DiffuseProbeGridRelocationPass::FrameEndInternal()
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGridsForUpdate(*this))
            {
                // relocation stops after a limited number of iterations
                diffuseProbeGrid->AdvanceRelocation();
            }

            RenderPass::FrameEndInternal();
        }
    }   // namespace Render
}   // namespace AZ
