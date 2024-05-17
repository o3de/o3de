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
#include <Atom/RHI/SingleDeviceDispatchRaysItem.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/SingleDevicePipelineState.h>
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

            m_passData = RPI::PassUtils::GetPassData<RayTracingPassData>(m_passDescriptor);
            if (m_passData == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Invalid RayTracingPassData", GetPathName().GetCStr());
                return;
            }

            CreatePipelineState();
        }

        RayTracingPass::~RayTracingPass()
        {
            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        void RayTracingPass::CreatePipelineState()
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            struct RTShaderLib
            {
                AZ::Data::AssetId m_shaderAssetId;
                AZ::Data::Instance<AZ::RPI::Shader> m_shader;
                AZ::RHI::PipelineStateDescriptorForRayTracing m_pipelineStateDescriptor;
                AZ::Name m_rayGenerationShaderName;
                AZ::Name m_missShaderName;
                AZ::Name m_closestHitShaderName;
                AZ::Name m_closestHitProceduralShaderName;
            };
            AZStd::fixed_vector<RTShaderLib, 4> shaderLibs;

            auto loadRayTracingShader = [&](auto& assetReference) -> RTShaderLib&
            {
                auto it = std::find_if(
                    shaderLibs.begin(),
                    shaderLibs.end(),
                    [&](auto& entry)
                    {
                        return entry.m_shaderAssetId == assetReference.m_assetId;
                    });
                if (it != shaderLibs.end())
                {
                    return *it;
                }
                auto shaderAsset{ AZ::RPI::FindShaderAsset(assetReference.m_assetId, assetReference.m_filePath) };
                AZ_Assert(shaderAsset.IsReady(), "Failed to load shader %s", assetReference.m_filePath.c_str());
                auto shader{ AZ::RPI::Shader::FindOrCreate(shaderAsset) };
                auto shaderVariant{ shader->GetVariant(AZ::RPI::ShaderAsset::RootShaderVariantStableId) };
                AZ::RHI::PipelineStateDescriptorForRayTracing pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
                auto& shaderLib = shaderLibs.emplace_back();
                shaderLib.m_shaderAssetId = assetReference.m_assetId;
                shaderLib.m_shader = shader;
                shaderLib.m_pipelineStateDescriptor = pipelineStateDescriptor;
                return shaderLib;
            };

            auto& rayGenShaderLib{ loadRayTracingShader(m_passData->m_rayGenerationShaderAssetReference) };
            rayGenShaderLib.m_rayGenerationShaderName = m_passData->m_rayGenerationShaderName;
            m_rayGenerationShader = rayGenShaderLib.m_shader;

            auto& closestHitShaderLib{ loadRayTracingShader(m_passData->m_closestHitShaderAssetReference) };
            closestHitShaderLib.m_closestHitShaderName = m_passData->m_closestHitShaderName;
            m_closestHitShader = closestHitShaderLib.m_shader;

            if (!m_passData->m_closestHitProceduralShaderName.empty())
            {
                auto& closestHitProceduralShaderLib{ loadRayTracingShader(m_passData->m_closestHitProceduralShaderAssetReference) };
                closestHitProceduralShaderLib.m_closestHitProceduralShaderName = m_passData->m_closestHitProceduralShaderName;
                m_closestHitProceduralShader = closestHitProceduralShaderLib.m_shader;
            }

            auto& missShaderLib{ loadRayTracingShader(m_passData->m_missShaderAssetReference) };
            missShaderLib.m_missShaderName = m_passData->m_missShaderName;
            m_missShader = missShaderLib.m_shader;

            m_globalPipelineState = m_rayGenerationShader->AcquirePipelineState(shaderLibs.front().m_pipelineStateDescriptor);
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

            const auto& rayTracingSceneSrgLayout = m_rayGenerationShader->FindShaderResourceGroupLayout(RayTracingSceneSrgBindingSlot);
            m_requiresRayTracingSceneSrg = (rayTracingSceneSrgLayout != nullptr);

            // build the ray tracing pipeline state descriptor
            RHI::SingleDeviceRayTracingPipelineStateDescriptor descriptor;
            descriptor.Build()
                ->PipelineState(m_globalPipelineState.get())
                ->MaxPayloadSize(m_passData->m_maxPayloadSize)
                ->MaxAttributeSize(m_passData->m_maxAttributeSize)
                ->MaxRecursionDepth(m_passData->m_maxRecursionDepth);
            for (auto& shaderLib : shaderLibs)
            {
                descriptor.ShaderLibrary(shaderLib.m_pipelineStateDescriptor);
                if (!shaderLib.m_rayGenerationShaderName.IsEmpty())
                {
                    descriptor.RayGenerationShaderName(AZ::Name{ m_passData->m_rayGenerationShaderName });
                }
                if (!shaderLib.m_closestHitShaderName.IsEmpty())
                {
                    descriptor.ClosestHitShaderName(AZ::Name{ m_passData->m_closestHitShaderName });
                }
                if (!shaderLib.m_closestHitProceduralShaderName.IsEmpty())
                {
                    descriptor.ClosestHitShaderName(AZ::Name{ m_passData->m_closestHitProceduralShaderName });
                }
                if (!shaderLib.m_missShaderName.IsEmpty())
                {
                    descriptor.MissShaderName(AZ::Name{ m_passData->m_missShaderName });
                }
            }
            descriptor.HitGroup(AZ::Name("HitGroup"))->ClosestHitShaderName(AZ::Name(m_passData->m_closestHitShaderName.c_str()));

            RayTracingFeatureProcessor* rayTracingFeatureProcessor =
                GetScene() ? GetScene()->GetFeatureProcessor<RayTracingFeatureProcessor>() : nullptr;
            if (rayTracingFeatureProcessor && !m_passData->m_closestHitProceduralShaderName.empty())
            {
                const auto& proceduralGeometryTypes = rayTracingFeatureProcessor->GetProceduralGeometryTypes();
                for (auto it = proceduralGeometryTypes.cbegin(); it != proceduralGeometryTypes.cend(); ++it)
                {
                    auto shaderVariant{ it->m_intersectionShader->GetVariant(AZ::RPI::ShaderAsset::RootShaderVariantStableId) };
                    AZ::RHI::PipelineStateDescriptorForRayTracing pipelineStateDescriptor;
                    shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
                    descriptor.ShaderLibrary(pipelineStateDescriptor);
                    descriptor.IntersectionShaderName(it->m_intersectionShaderName);

                    descriptor.HitGroup(it->m_name)
                        ->ClosestHitShaderName(AZ::Name(m_passData->m_closestHitProceduralShaderName))
                        ->IntersectionShaderName(it->m_intersectionShaderName);
                }
            }

            // create the ray tracing pipeline state object
            m_rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
            m_rayTracingPipelineState->Init(*device.get(), &descriptor);

            // make sure the shader table rebuilds if we're hotreloading
            m_rayTracingRevision = 0;

            // store the max ray length
            m_maxRayLength = m_passData->m_maxRayLength;

            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_rayGenerationShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_closestHitShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_closestHitProceduralShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_missShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_intersectionShaderAssetReference.m_assetId);
        }

        Data::Instance<RPI::Shader> RayTracingPass::LoadShader(const RPI::AssetReference& shaderAssetReference)
        {
            Data::Asset<RPI::ShaderAsset> shaderAsset;
            if (shaderAssetReference.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(shaderAssetReference.m_assetId, shaderAssetReference.m_filePath);
            }

            if (!shaderAsset.IsReady())
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Failed to load shader asset [%s]", GetPathName().GetCStr(), shaderAssetReference.m_filePath.data());
                return nullptr;
            }

            return RPI::Shader::FindOrCreate(shaderAsset);
        }

        bool RayTracingPass::IsEnabled() const
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
            if (!rayTracingFeatureProcessor)
            {
                return false;
            }

            return true;
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
                RHI::SingleDeviceRayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();

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
                const RHI::Ptr<RHI::SingleDeviceBuffer>& rayTracingTlasBuffer = rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer();
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
                auto constantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_maxRayLength"));
                if (constantIndex.IsValid())
                {
                    m_shaderResourceGroup->SetConstant(constantIndex, m_maxRayLength);
                }

                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
            }

            uint32_t proceduralGeometryTypeRevision = rayTracingFeatureProcessor->GetProceduralGeometryTypeRevision();
            if (m_proceduralGeometryTypeRevision != proceduralGeometryTypeRevision)
            {
                CreatePipelineState();
                RPI::SceneNotificationBus::Event(
                    GetScene()->GetId(),
                    &RPI::SceneNotification::OnRenderPipelineChanged,
                    GetRenderPipeline(),
                    RPI::SceneNotification::RenderPipelineChangeType::PassChanged);
                m_proceduralGeometryTypeRevision = proceduralGeometryTypeRevision;
            }

            uint32_t rayTracingRevision = rayTracingFeatureProcessor->GetRevision();
            if (m_rayTracingRevision != rayTracingRevision)
            {
                // scene changed, need to rebuild the shader table
                m_rayTracingRevision = rayTracingRevision;

                AZStd::shared_ptr<RHI::SingleDeviceRayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<RHI::SingleDeviceRayTracingShaderTableDescriptor>();

                if (rayTracingFeatureProcessor->HasGeometry())
                {
                    // build the ray tracing shader table descriptor
                    RHI::SingleDeviceRayTracingShaderTableDescriptor* descriptorBuild = descriptor->Build(AZ::Name("RayTracingShaderTable"), m_rayTracingPipelineState)
                        ->RayGenerationRecord(AZ::Name(m_passData->m_rayGenerationShaderName.c_str()))
                        ->MissRecord(AZ::Name(m_passData->m_missShaderName.c_str()));

                    // add a hit group for standard meshes mesh to the shader table
                    descriptorBuild->HitGroupRecord(AZ::Name("HitGroup"));

                    // add a hit group for each procedural geometry type to the shader table
                    const auto& proceduralGeometryTypes = rayTracingFeatureProcessor->GetProceduralGeometryTypes();
                    for (auto it = proceduralGeometryTypes.cbegin(); it != proceduralGeometryTypes.cend(); ++it)
                    {
                        descriptorBuild->HitGroupRecord(it->m_name);
                        // TODO(intersection): Set per-hitgroup SRG once RayTracingPipelineState supports local root signatures
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
                !rayTracingFeatureProcessor->HasGeometry() ||
                !m_rayTracingShaderTable)
            {
                return;
            }

            RHI::SingleDeviceDispatchRaysItem dispatchRaysItem;

            // calculate thread counts if this is a full screen raytracing pass
            if (m_passData->m_makeFullscreenPass)
            {
                RPI::PassAttachment* outputAttachment = nullptr;

                if (GetOutputCount() > 0)
                {
                    outputAttachment = GetOutputBinding(0).GetAttachment().get();
                }
                else if (GetInputOutputCount() > 0)
                {
                    outputAttachment = GetInputOutputBinding(0).GetAttachment().get();
                }

                AZ_Assert(outputAttachment != nullptr, "[RayTracingPass '%s']: A fullscreen RayTracing pass must have a valid output or input/output.", GetPathName().GetCStr());
                AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image, "[RayTracingPass '%s']: The output of a fullscreen RayTracing pass must be an image.", GetPathName().GetCStr());

                RHI::Size imageSize = outputAttachment->m_descriptor.m_image.m_size;

                dispatchRaysItem.m_arguments.m_direct.m_width = imageSize.m_width;
                dispatchRaysItem.m_arguments.m_direct.m_height = imageSize.m_height;
                dispatchRaysItem.m_arguments.m_direct.m_depth = imageSize.m_depth;
            }
            else
            {
                dispatchRaysItem.m_arguments.m_direct.m_width = m_passData->m_threadCountX;
                dispatchRaysItem.m_arguments.m_direct.m_height = m_passData->m_threadCountY;
                dispatchRaysItem.m_arguments.m_direct.m_depth = m_passData->m_threadCountZ;
            }

            // bind RayTracingGlobal, RayTracingScene, and View Srgs
            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            AZStd::vector<RHI::SingleDeviceShaderResourceGroup*> shaderResourceGroups = { m_shaderResourceGroup->GetRHIShaderResourceGroup() };

            if (m_requiresRayTracingSceneSrg)
            {
                shaderResourceGroups.push_back(rayTracingFeatureProcessor->GetRayTracingSceneSrg()->GetRHIShaderResourceGroup());
            }

            if (m_requiresViewSrg)
            {
                RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
                if (view)
                {
                    shaderResourceGroups.push_back(view->GetRHIShaderResourceGroup());
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
            CreatePipelineState();
        }

        void RayTracingPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            CreatePipelineState();
        }

        void RayTracingPass::OnShaderVariantReinitialized(const RPI::ShaderVariant&)
        {
            CreatePipelineState();
        }
    }   // namespace Render
}   // namespace AZ
