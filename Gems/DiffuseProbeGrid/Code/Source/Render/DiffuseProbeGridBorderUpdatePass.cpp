/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseProbeGridBorderUpdatePass.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridBorderUpdatePass> DiffuseProbeGridBorderUpdatePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridBorderUpdatePass> pass = aznew DiffuseProbeGridBorderUpdatePass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridBorderUpdatePass::DiffuseProbeGridBorderUpdatePass(const RPI::PassDescriptor& descriptor)
            : RPI::RenderPass(descriptor)
        {
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
            }
            else
            {
                LoadShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridBorderUpdateRow.azshader",
                           m_rowShader,
                           m_rowPipelineState,
                           m_rowSrgLayout,
                           m_rowDispatchArgs);

                LoadShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridBorderUpdateColumn.azshader",
                           m_columnShader,
                           m_columnPipelineState,
                           m_columnSrgLayout,
                           m_columnDispatchArgs);
            }
        }

        void DiffuseProbeGridBorderUpdatePass::LoadShader(AZStd::string shaderFilePath,
                                                          Data::Instance<RPI::Shader>& shader,
                                                          const RHI::PipelineState*& pipelineState,
                                                          RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout,
                                                          RHI::DispatchDirect& dispatchArgs)
        {
            // load shader
            // Note: the shader may not be available on all platforms
            shader = RPI::LoadCriticalShader(shaderFilePath);
            if (shader == nullptr)
            {
                return;
            }

            // load pipeline state
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);

            // load Pass Srg asset
            srgLayout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = RPI::GetComputeShaderNumThreads(shader->GetAsset(), dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeGridBorderUpdatePass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridBorderUpdatePass::IsEnabled() const
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

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
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

        void DiffuseProbeGridBorderUpdatePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // there are 4 submits per grid
            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().size() * 4));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // probe irradiance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe distance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }
            }
        }

        void DiffuseProbeGridBorderUpdatePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see line ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateBorderUpdateSrgs(m_rowShader, m_rowSrgLayout, m_columnShader, m_columnSrgLayout);

                diffuseProbeGrid->GetBorderUpdateRowIrradianceSrg()->Compile();
                diffuseProbeGrid->GetBorderUpdateColumnIrradianceSrg()->Compile();
                diffuseProbeGrid->GetBorderUpdateRowDistanceSrg()->Compile();
                diffuseProbeGrid->GetBorderUpdateColumnDistanceSrg()->Compile();
            }
        }

        void DiffuseProbeGridBorderUpdatePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            uint32_t index = context.GetSubmitRange().m_startIndex;
            while (index < context.GetSubmitRange().m_endIndex)
            {
                // Note: there are 4 submits per grid, so we need to calculate the diffuseProbeGridIndex in the list as (submit index / 4)
                AZ_Assert(index % 4 == 0, "Incorrect number of submits in DiffuseProbeGridBorderUpdatePass::BuildCommandListInternal");
                uint32_t diffuseProbeGridIndex = index / 4;
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids()[diffuseProbeGridIndex];

                uint32_t probeCountX;
                uint32_t probeCountY;
                diffuseProbeGrid->GetTexture2DProbeCount(probeCountX, probeCountY);

                // row irradiance
                {
                    const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateRowIrradianceSrg()->GetRHIShaderResourceGroup();
                    commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                    RHI::DispatchItem dispatchItem;
                    dispatchItem.m_arguments = m_rowDispatchArgs;
                    dispatchItem.m_pipelineState = m_rowPipelineState;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX * (DiffuseProbeGrid::DefaultNumIrradianceTexels + 2);
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                    commandList->Submit(dispatchItem, index++);
                }

                // column irradiance
                {
                    const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateColumnIrradianceSrg()->GetRHIShaderResourceGroup();
                    commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                    RHI::DispatchItem dispatchItem;
                    dispatchItem.m_arguments = m_columnDispatchArgs;
                    dispatchItem.m_pipelineState = m_columnPipelineState;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY * (DiffuseProbeGrid::DefaultNumIrradianceTexels + 2);
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                    commandList->Submit(dispatchItem, index++);
                }

                // row distance
                {
                    const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateRowDistanceSrg()->GetRHIShaderResourceGroup();
                    commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                    RHI::DispatchItem dispatchItem;
                    dispatchItem.m_arguments = m_rowDispatchArgs;
                    dispatchItem.m_pipelineState = m_rowPipelineState;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX * (DiffuseProbeGrid::DefaultNumDistanceTexels + 2);
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                    commandList->Submit(dispatchItem, index++);
                }

                // column distance
                {
                    const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateColumnDistanceSrg()->GetRHIShaderResourceGroup();
                    commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                    RHI::DispatchItem dispatchItem;
                    dispatchItem.m_arguments = m_columnDispatchArgs;
                    dispatchItem.m_pipelineState = m_columnPipelineState;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = probeCountX;
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = probeCountY * (DiffuseProbeGrid::DefaultNumDistanceTexels + 2);
                    dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                    commandList->Submit(dispatchItem, index++);
                }
            }
        }
    }   // namespace Render
}   // namespace AZ
