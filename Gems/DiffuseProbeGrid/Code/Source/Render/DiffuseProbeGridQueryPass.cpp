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
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridQueryPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridQueryPass> DiffuseProbeGridQueryPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridQueryPass> pass = aznew DiffuseProbeGridQueryPass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridQueryPass::DiffuseProbeGridQueryPass(const RPI::PassDescriptor& descriptor)
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

        void DiffuseProbeGridQueryPass::LoadShader()
        {
            // load shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridQuery.azshader";
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

            // load Srg layout
            m_srgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeGridQueryPass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridQueryPass::IsEnabled() const
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
            if (!diffuseProbeGridFeatureProcessor || !diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount())
            {
                // no irradiance queries
                return false;
            }

            return true;
        }

        void DiffuseProbeGridQueryPass::BuildInternal()
        {
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_outputBufferAttachmentId = AZStd::string::format("DiffuseProbeGridQueryOutputBuffer_%s", uuidString.c_str());

            // setup output PassAttachment
            m_outputAttachment = aznew RPI::PassAttachment();
            m_outputAttachment->m_name = "Output";
            AZ::Name attachmentPath(AZStd::string::format("%s.%s", GetPathName().GetCStr(), m_outputAttachment->m_name.GetCStr()));
            m_outputAttachment->m_path = attachmentPath;
            m_outputAttachment->m_lifetime = RHI::AttachmentLifetimeType::Transient;

            RPI::PassAttachmentBinding* outputBinding = FindAttachmentBinding(AZ::Name("Output"));
            AZ_Assert(outputBinding, "Failed to find Output slot on DiffuseProbeGridQueryPass");
            outputBinding->SetAttachment(m_outputAttachment);
        }

        void DiffuseProbeGridQueryPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // create output buffer descriptors
            m_outputBufferDesc.m_byteCount = diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount() * sizeof(AZ::Vector4);
            m_outputBufferDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
            m_outputBufferViewDesc = RHI::BufferViewDescriptor::CreateTyped(0, diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount(), RHI::Format::R32G32B32A32_FLOAT);

            m_outputAttachment->m_descriptor = m_outputBufferDesc;

            // create transient buffer
            RHI::TransientBufferDescriptor transientBufferDesc;
            transientBufferDesc.m_attachmentId = m_outputBufferAttachmentId;
            transientBufferDesc.m_bufferDescriptor = m_outputBufferDesc;
            params.m_frameGraphBuilder->GetAttachmentDatabase().CreateTransientBuffer(transientBufferDesc);

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridQueryPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().size()));

            // query buffer
            {
                RHI::AttachmentId attachmentId = diffuseProbeGridFeatureProcessor->GetQueryBufferAttachmentId();
                RHI::Ptr<RHI::Buffer> buffer = diffuseProbeGridFeatureProcessor->GetQueryBuffer()->GetRHIBuffer();

                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                {
                    [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(attachmentId, buffer);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to import query buffer");
                }

                RHI::BufferScopeAttachmentDescriptor desc;
                desc.m_attachmentId = attachmentId;
                desc.m_bufferViewDescriptor = diffuseProbeGridFeatureProcessor->GetQueryBufferViewDescriptor();
                desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
            }

            // output buffer
            {                      
                RHI::BufferScopeAttachmentDescriptor desc;
                desc.m_attachmentId = m_outputBufferAttachmentId;
                desc.m_bufferViewDescriptor = m_outputBufferViewDesc;
                desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Clear;
                desc.m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);

                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
            }

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

        void DiffuseProbeGridQueryPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // update DiffuseProbeGrid-specific bindings
                diffuseProbeGrid->UpdateQuerySrg(m_shader, m_srgLayout);

                // bind query buffer
                RHI::ShaderInputBufferIndex bufferIndex = m_srgLayout->FindShaderInputBufferIndex(AZ::Name("m_irradianceQueries"));
                RHI::Ptr<RHI::Buffer> buffer = diffuseProbeGridFeatureProcessor->GetQueryBuffer()->GetRHIBuffer();
                RHI::BufferViewDescriptor bufferViewDescriptor = diffuseProbeGridFeatureProcessor->GetQueryBufferViewDescriptor();
                diffuseProbeGrid->GetQuerySrg()->SetBufferView(bufferIndex, buffer->BuildBufferView(bufferViewDescriptor).get());

                // bind output UAV
                bufferIndex = m_srgLayout->FindShaderInputBufferIndex(AZ::Name("m_output"));
                const RHI::BufferView* bufferView = context.GetBufferView(AZ::Name(m_outputBufferAttachmentId.GetCStr()), RHI::ScopeAttachmentUsage::Shader);
                diffuseProbeGrid->GetQuerySrg()->SetBufferView(bufferIndex, bufferView);

                diffuseProbeGrid->GetQuerySrg()->Compile();
            }
        }

        void DiffuseProbeGridQueryPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids()[index];

                const RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetQuerySrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

                RHI::DeviceDispatchItem dispatchItem;
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_pipelineState = m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 1;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }

        void DiffuseProbeGridQueryPass::FrameEndInternal()
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            diffuseProbeGridFeatureProcessor->ClearIrradianceQueries();
                
            RenderPass::FrameEndInternal();
        }
    }   // namespace Render
}   // namespace AZ
