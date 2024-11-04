/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridBlendIrradiancePass.h>

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
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
            }
            else
            {
                LoadShader();
            }
        }

        void DiffuseProbeGridBlendIrradiancePass::LoadShader()
        {
            // load shaders, each supervariant handles a different number of rays per probe
            // Note: the raytracing shaders may not be available on all platforms
            m_shaders.reserve(DiffuseProbeGridNumRaysPerProbeArraySize);
            for (uint32_t index = 0; index < DiffuseProbeGridNumRaysPerProbeArraySize; ++index)
            {
                AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridBlendIrradiance.azshader";
                Data::Instance<RPI::Shader> shader = RPI::LoadCriticalShader(shaderFilePath, DiffuseProbeGridNumRaysPerProbeArray[index].m_supervariant);
                if (shader == nullptr)
                {
                    return;
                }

                RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
                const auto& shaderVariant = shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shader->GetDefaultShaderOptions());
                const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
                AZ_Assert(pipelineState, "Failed to acquire pipeline state");

                RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
                AZ_Assert(srgLayout.get(), "Failed to find Srg layout");

                RHI::DispatchDirect dispatchArgs;
                const auto outcome = RPI::GetComputeShaderNumThreads(shader->GetAsset(), dispatchArgs);
                if (!outcome.IsSuccess())
                {
                    AZ_Error("PassSystem", false, "[DiffuseProbeBlendIrradiancePass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
                }

                m_shaders.push_back({ shader, pipelineState, srgLayout, dispatchArgs });
            }
        }

        bool DiffuseProbeGridBlendIrradiancePass::IsEnabled() const
        {
            if (!RenderPass::IsEnabled())
            {
                return false;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessorInterface>();
            if (!rayTracingFeatureProcessor || !rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // empty scene
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().empty())
            {
                // no diffuse probe grids
                return false;
            }

            return true;
        }

        void DiffuseProbeGridBlendIrradiancePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().size()));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
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

                // probe irradiance
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
                }
            }
        }

        void DiffuseProbeGridBlendIrradiancePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() in ShaderResourceGroupData.cpp)
                DiffuseProbeGridShader& shader = m_shaders[diffuseProbeGrid->GetNumRaysPerProbe().m_index];
                diffuseProbeGrid->UpdateBlendIrradianceSrg(shader.m_shader, shader.m_srgLayout);

                if (!diffuseProbeGrid->GetBlendIrradianceSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetBlendIrradianceSrg()->Compile();
                }
            }
        }

        void DiffuseProbeGridBlendIrradiancePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids()[index];
                DiffuseProbeGridShader& shader = m_shaders[diffuseProbeGrid->GetNumRaysPerProbe().m_index];

                const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetBlendIrradianceSrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get());

                uint32_t probeCountX;
                uint32_t probeCountY;
                diffuseProbeGrid->GetTexture2DProbeCount(probeCountX, probeCountY);

                probeCountX = AZ::DivideAndRoundUp(probeCountX, diffuseProbeGrid->GetFrameUpdateCount());

                RHI::DeviceDispatchItem dispatchItem;
                dispatchItem.m_arguments = shader.m_dispatchArgs;
                dispatchItem.m_pipelineState = shader.m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX * dispatchItem.m_arguments.m_direct.m_threadsPerGroupX;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY * dispatchItem.m_arguments.m_direct.m_threadsPerGroupY;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }
    }   // namespace Render
}   // namespace AZ
