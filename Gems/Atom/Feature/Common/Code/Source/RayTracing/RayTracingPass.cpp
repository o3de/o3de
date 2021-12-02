/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <RayTracing/RayTracingPass.h>
#include <RayTracing/RayTracingPassData.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<RayTracingPass> RayTracingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RayTracingPass> pass = aznew RayTracingPass(descriptor);
            return pass;
        }

        RayTracingPass::RayTracingPass(const RPI::PassDescriptor& descriptor)
            : RenderPass(descriptor)
            , m_passDescriptor(descriptor)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false)
            {
                // raytracing is not supported on this platform
                SetEnabled(false);
                return;
            }

            Init();
        }

        RayTracingPass::~RayTracingPass()
        {
            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        void RayTracingPass::Init()
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            m_passData = RPI::PassUtils::GetPassData<RayTracingPassData>(m_passDescriptor);
            if (m_passData == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Invalid RayTracingPassData", GetPathName().GetCStr());
                return;
            }

            // ray generation shader
            m_rayGenerationShader = LoadShader(m_passData->m_rayGenerationShaderAssetReference);
            if (m_rayGenerationShader == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Failed to load RayGeneration shader [%s]", GetPathName().GetCStr(), m_passData->m_rayGenerationShaderAssetReference.m_filePath.data());
                return;
            }

            auto shaderVariant = m_rayGenerationShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForRayTracing rayGenerationShaderDescriptor;
            shaderVariant.ConfigurePipelineState(rayGenerationShaderDescriptor);

            // closest hit shader
            m_closestHitShader = LoadShader(m_passData->m_closestHitShaderAssetReference);
            if (m_closestHitShader == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Failed to load ClosestHit shader [%s]", GetPathName().GetCStr(), m_passData->m_closestHitShaderAssetReference.m_filePath.data());
                return;
            }

            shaderVariant = m_closestHitShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForRayTracing closestHitShaderDescriptor;
            shaderVariant.ConfigurePipelineState(closestHitShaderDescriptor);

            // miss shader
            m_missShader = LoadShader(m_passData->m_missShaderAssetReference);
            if (m_missShader == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Failed to load Miss shader [%s]", GetPathName().GetCStr(), m_passData->m_missShaderAssetReference.m_filePath.data());
                return;
            }

            shaderVariant = m_missShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForRayTracing missShaderDescriptor;
            shaderVariant.ConfigurePipelineState(missShaderDescriptor);

            // retrieve global pipeline state
            m_globalPipelineState = m_rayGenerationShader->AcquirePipelineState(rayGenerationShaderDescriptor);
            AZ_Assert(m_globalPipelineState, "Failed to acquire ray tracing global pipeline state");

            // create global srg
            const auto& globalSrgLayout = m_rayGenerationShader->FindShaderResourceGroupLayout(RayTracingGlobalSrgBindingSlot);
            AZ_Error("PassSystem", globalSrgLayout != nullptr, "RayTracingPass [%s] Failed to find RayTracingGlobalSrg layout", GetPathName().GetCStr());

            m_shaderResourceGroup = RPI::ShaderResourceGroup::Create(  m_rayGenerationShader->GetAsset(), m_rayGenerationShader->GetSupervariantIndex(), globalSrgLayout->GetName());
            AZ_Assert(m_shaderResourceGroup, "RayTracingPass [%s]: Failed to create RayTracingGlobalSrg", GetPathName().GetCStr());
            RPI::PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());

            // check to see if the shader requires the View, Scene, or RayTracingMaterial Srgs
            const auto& viewSrgLayout = m_rayGenerationShader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::View);
            m_requiresViewSrg = (viewSrgLayout != nullptr);

            const auto& sceneSrgLayout = m_rayGenerationShader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Scene);
            m_requiresSceneSrg = (sceneSrgLayout != nullptr);

            const auto& rayTracingMaterialSrgLayout = m_rayGenerationShader->FindShaderResourceGroupLayout(RayTracingMaterialSrgBindingSlot);
            m_requiresRayTracingMaterialSrg = (rayTracingMaterialSrgLayout != nullptr);

            // build the ray tracing pipeline state descriptor
            RHI::RayTracingPipelineStateDescriptor descriptor;
            descriptor.Build()
                ->PipelineState(m_globalPipelineState.get())
                ->MaxPayloadSize(m_passData->m_maxPayloadSize)
                ->MaxAttributeSize(m_passData->m_maxAttributeSize)
                ->MaxRecursionDepth(m_passData->m_maxRecursionDepth)
                ->ShaderLibrary(rayGenerationShaderDescriptor)
                    ->RayGenerationShaderName(AZ::Name(m_passData->m_rayGenerationShaderName.c_str()))
                ->ShaderLibrary(missShaderDescriptor)
                    ->MissShaderName(AZ::Name(m_passData->m_missShaderName.c_str()))
                ->ShaderLibrary(closestHitShaderDescriptor)
                    ->ClosestHitShaderName(AZ::Name(m_passData->m_closestHitShaderName.c_str()))
                ->HitGroup(AZ::Name("HitGroup"))
                    ->ClosestHitShaderName(AZ::Name(m_passData->m_closestHitShaderName.c_str()));

            // create the ray tracing pipeline state object
            m_rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
            m_rayTracingPipelineState->Init(*device.get(), &descriptor);

            // make sure the shader table rebuilds if we're hotreloading
            m_rayTracingRevision = 0;

            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_rayGenerationShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_closestHitShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_missShaderAssetReference.m_assetId);
        }

        Data::Instance<RPI::Shader> RayTracingPass::LoadShader(const RPI::AssetReference& shaderAssetReference)
        {
            Data::Asset<RPI::ShaderAsset> shaderAsset;
            if (shaderAssetReference.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(shaderAssetReference.m_assetId, shaderAssetReference.m_filePath);
            }

            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Failed to load shader asset [%s]", GetPathName().GetCStr(), shaderAssetReference.m_filePath.data());
                return nullptr;
            }

            return RPI::Shader::FindOrCreate(shaderAsset);
        }

        void RayTracingPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (!rayTracingFeatureProcessor)
            {
                return;
            }

            if (!m_rayTracingShaderTable)
            {
                RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
                RHI::RayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();

                m_rayTracingShaderTable = RHI::Factory::Get().CreateRayTracingShaderTable();
                m_rayTracingShaderTable->Init(*device.get(), rayTracingBufferPools);
            }

            RPI::RenderPass::FrameBeginInternal(params);
        }

        void RayTracingPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
            frameGraph.SetEstimatedItemCount(1);

            // TLAS
            {
                const RHI::Ptr<RHI::Buffer>& rayTracingTlasBuffer = rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer();
                if (rayTracingTlasBuffer)
                {
                    AZ::RHI::AttachmentId tlasAttachmentId = rayTracingFeatureProcessor->GetTlasAttachmentId();
                    if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(tlasAttachmentId) == false)
                    {
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, rayTracingTlasBuffer);
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);
                    }

                    uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer()->GetDescriptor().m_byteCount);
                    RHI::BufferViewDescriptor tlasBufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, tlasBufferByteCount);

                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = tlasAttachmentId;
                    desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }
            }
        }

        void RayTracingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
            }

            uint32_t rayTracingRevision = rayTracingFeatureProcessor->GetRevision();
            if (m_rayTracingRevision != rayTracingRevision)
            {
                // scene changed, need to rebuild the shader table
                m_rayTracingRevision = rayTracingRevision;

                AZStd::shared_ptr<RHI::RayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<RHI::RayTracingShaderTableDescriptor>();

                if (rayTracingFeatureProcessor->GetSubMeshCount())
                {
                    // build the ray tracing shader table descriptor
                    RHI::RayTracingShaderTableDescriptor* descriptorBuild = descriptor->Build(AZ::Name("RayTracingShaderTable"), m_rayTracingPipelineState)
                        ->RayGenerationRecord(AZ::Name(m_passData->m_rayGenerationShaderName.c_str()))
                        ->MissRecord(AZ::Name(m_passData->m_missShaderName.c_str()));

                    // add a hit group for each mesh to the shader table
                    for (uint32_t i = 0; i < rayTracingFeatureProcessor->GetSubMeshCount(); ++i)
                    {
                        descriptorBuild->HitGroupRecord(AZ::Name("HitGroup"));
                    }
                }

                m_rayTracingShaderTable->Build(descriptor);
            }
        }

        void RayTracingPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor ||
                !rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() ||
                !rayTracingFeatureProcessor->GetSubMeshCount() ||
                !m_rayTracingShaderTable)
            {
                return;
            }

            RHI::DispatchRaysItem dispatchRaysItem;

            // calculate thread counts if this is a full screen raytracing pass
            if (m_passData->m_makeFullscreenPass)
            {
                RPI::PassAttachment* outputAttachment = nullptr;

                if (GetOutputCount() > 0)
                {
                    outputAttachment = GetOutputBinding(0).m_attachment.get();
                }
                else if (GetInputOutputCount() > 0)
                {
                    outputAttachment = GetInputOutputBinding(0).m_attachment.get();
                }

                AZ_Assert(outputAttachment != nullptr, "[RayTracingPass '%s']: A fullscreen RayTracing pass must have a valid output or input/output.", GetPathName().GetCStr());
                AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image, "[RayTracingPass '%s']: The output of a fullscreen RayTracing pass must be an image.", GetPathName().GetCStr());

                RHI::Size imageSize = outputAttachment->m_descriptor.m_image.m_size;

                dispatchRaysItem.m_width = imageSize.m_width;
                dispatchRaysItem.m_height = imageSize.m_height;
                dispatchRaysItem.m_depth = imageSize.m_depth;
            }
            else
            {
                dispatchRaysItem.m_width = m_passData->m_threadCountX;
                dispatchRaysItem.m_height = m_passData->m_threadCountY;
                dispatchRaysItem.m_depth = m_passData->m_threadCountZ;
            }

            // bind RayTracingGlobal, RayTracingScene, and View Srgs
            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            AZStd::vector<RHI::ShaderResourceGroup*> shaderResourceGroups =
            {
                m_shaderResourceGroup->GetRHIShaderResourceGroup(),
                rayTracingFeatureProcessor->GetRayTracingSceneSrg()->GetRHIShaderResourceGroup()
            };

            if (m_requiresViewSrg)
            {
                const AZStd::vector<RPI::ViewPtr>& views = m_pipeline->GetViews(m_passData->m_pipelineViewTag);
                if (views.size() > 0)
                {
                    shaderResourceGroups.push_back(views[0]->GetRHIShaderResourceGroup());
                }
            }

            if (m_requiresSceneSrg)
            {
                shaderResourceGroups.push_back(scene->GetShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            if (m_requiresRayTracingMaterialSrg)
            {
                shaderResourceGroups.push_back(rayTracingFeatureProcessor->GetRayTracingMaterialSrg()->GetRHIShaderResourceGroup());
            }

            dispatchRaysItem.m_shaderResourceGroupCount = aznumeric_cast<uint32_t>(shaderResourceGroups.size());
            dispatchRaysItem.m_shaderResourceGroups = shaderResourceGroups.data();
            dispatchRaysItem.m_rayTracingPipelineState = m_rayTracingPipelineState.get();
            dispatchRaysItem.m_rayTracingShaderTable = m_rayTracingShaderTable.get();
            dispatchRaysItem.m_globalPipelineState = m_globalPipelineState.get();

            // submit the DispatchRays item
            context.GetCommandList()->Submit(dispatchRaysItem);
        }

        void RayTracingPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader& shader)
        {
            Init();
        }

        void RayTracingPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            Init();
        }

        void RayTracingPass::OnShaderVariantReinitialized(const RPI::ShaderVariant&)
        {
            Init();
        }
    }   // namespace Render
}   // namespace AZ
