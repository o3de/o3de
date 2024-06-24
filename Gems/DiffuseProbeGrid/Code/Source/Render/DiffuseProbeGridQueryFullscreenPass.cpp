/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridQueryFullscreenPass.h>
#include <Render/DiffuseProbeGridQueryFullscreenPassData.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridQueryFullscreenPass> DiffuseProbeGridQueryFullscreenPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridQueryFullscreenPass> pass = aznew DiffuseProbeGridQueryFullscreenPass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridQueryFullscreenPass::DiffuseProbeGridQueryFullscreenPass(const RPI::PassDescriptor& descriptor)
            : RPI::RenderPass(descriptor),
              m_passDescriptor(descriptor)
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

        void DiffuseProbeGridQueryFullscreenPass::LoadShader()
        {
            auto passData = RPI::PassUtils::GetPassData<DiffuseProbeGridQueryFullscreenPassData>(m_passDescriptor);
            bool useAlbedoTexture = (passData && passData->m_useAlbedoTexture);

            // load shader
            // Note: the shader may not be available on all platforms
            static const char* QueryFullscreenShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridQueryFullscreen.azshader";
            static const char* QueryFullscreenWithAlbedoShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridQueryFullscreenWithAlbedo.azshader";
            AZStd::string shaderFilePath = useAlbedoTexture ? QueryFullscreenWithAlbedoShaderFilePath : QueryFullscreenShaderFilePath;
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

            // load the ObjectSrg layout
            m_objectSrgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object);

            // load the PassSrg
            const auto passSrgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
            if (passSrgLayout)
            {
                m_shaderResourceGroup = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), passSrgLayout->GetName());
                AZ_Assert(m_shaderResourceGroup, "[DiffuseProbeGridQueryFullscreenPass]: Failed to create PassSrg", GetPathName().GetCStr());

                RPI::PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());
            }

            // retrieve the number of threads per thread group from the shader
            const auto outcome = RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeGridQueryFullscreenPass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridQueryFullscreenPass::IsEnabled() const
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

            // Note: the pass is enabled even if none of the queries are in a DiffuseProbeGrid volume.  This is necessary to provide a zero result
            // for those queries in the transient output buffer.
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor)
            {
                return false;
            }

            return true;
        }

        void DiffuseProbeGridQueryFullscreenPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().size()));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // grid data buffer
                {
                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe irradiance
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe distance
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe data
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                }
            }
        }

        void DiffuseProbeGridQueryFullscreenPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // update DiffuseProbeGrid-specific bindings
                diffuseProbeGrid->UpdateQuerySrg(m_shader, m_objectSrgLayout);

                diffuseProbeGrid->GetQuerySrg()->Compile();
            }

            if (auto viewSRG = diffuseProbeGridFeatureProcessor->GetViewSrg(m_pipeline, GetPipelineViewTag()))
            {
                BindSrg(viewSRG->GetRHIShaderResourceGroup());
            }
        }

        void DiffuseProbeGridQueryFullscreenPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids()[index];

                const AZStd::vector<RPI::ViewPtr>& views = m_pipeline->GetViews(RPI::PipelineViewTag{ "MainCamera" });
                if (views.empty())
                {
                    return;
                }

                // retrieve output image for the number of dispatch threads
                RPI::PassAttachment* outputAttachment = nullptr;
                if (GetOutputCount() > 0)
                {
                    outputAttachment = GetOutputBinding(0).GetAttachment().get();
                }
                else if (GetInputOutputCount() > 0)
                {
                    outputAttachment = GetInputOutputBinding(0).GetAttachment().get();
                }

                AZ_Assert(outputAttachment != nullptr, "[DiffuseProbeGridQueryFullscreenPass '%s']: A fullscreen DiffuseProbeGridQuery pass must have a valid output or input/output.", GetPathName().GetCStr());
                AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image, "[DiffuseProbeGridQueryFullscreenPass '%s']: The output of a fullscreen DiffuseProbeGridQuery pass must be an image.", GetPathName().GetCStr());

                RHI::Size imageSize = outputAttachment->m_descriptor.m_image.m_size;

                // submit DispatchItem
                const uint8_t srgCount = 3;
                AZStd::array<const RHI::DeviceShaderResourceGroup*, 8> shaderResourceGroups =
                {
                    diffuseProbeGrid->GetQuerySrg()->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                    m_shaderResourceGroup->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                    views[0]->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get()
                };

                RHI::DeviceDispatchItem dispatchItem;
                dispatchItem.m_pipelineState = m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_shaderResourceGroupCount = srgCount;
                dispatchItem.m_shaderResourceGroups = shaderResourceGroups;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = imageSize.m_width;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = imageSize.m_height;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }
    }   // namespace Render
}   // namespace AZ
