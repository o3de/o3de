/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/Feature/RayTracing/RayTracingPassData.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

using uint = uint32_t;
using uint4 = uint[4];
#include "../../../Feature/Common/Assets/ShaderLib/Atom/Features/IndirectRendering.azsli"

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
            , m_dispatchRaysItem(RHI::RHISystemInterface::Get()->GetRayTracingSupport())
        {
            m_flags.m_canBecomeASubpass = false;
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices)
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

            m_indirectDispatch = m_passData->m_indirectDispatch;
            m_indirectDispatchBufferSlotName = m_passData->m_indirectDispatchBufferSlotName;

            m_fullscreenDispatch = m_passData->m_fullscreenDispatch;
            m_fullscreenSizeSourceSlotName = m_passData->m_fullscreenSizeSourceSlotName;

            AZ_Assert(
                !(m_indirectDispatch && m_fullscreenDispatch),
                "[RaytracingPass '%s']: Only one of the dispatch options (indirect, fullscreen) can be active",
                GetPathName().GetCStr());

            m_defaultShaderAttachmentStage = RHI::ScopeAttachmentStage::RayTracingShader;
            CreatePipelineState();
        }

        RayTracingPass::~RayTracingPass()
        {
            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        void RayTracingPass::CreatePipelineState()
        {
            m_rayTracingShaderTable.reset();
            m_maxRayLengthInputIndex.Reset();

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

            auto loadRayTracingShader = [&](auto& assetReference, const AZ::Name& supervariantName = AZ::Name("")) -> RTShaderLib&
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
                auto shader{ AZ::RPI::Shader::FindOrCreate(shaderAsset, supervariantName) };
                auto shaderVariant{ shader->GetVariant(AZ::RPI::ShaderAsset::RootShaderVariantStableId) };
                AZ::RHI::PipelineStateDescriptorForRayTracing pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shader->GetDefaultShaderOptions());
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
                auto& closestHitProceduralShaderLib{ loadRayTracingShader(
                    m_passData->m_closestHitProceduralShaderAssetReference, AZ::RHI::GetDefaultSupervariantNameWithNoFloat16Fallback()) };
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
            RHI::RayTracingPipelineStateDescriptor descriptor;
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
            m_rayTracingPipelineState = aznew RHI::RayTracingPipelineState;
            m_rayTracingPipelineState->Init(RHI::RHISystemInterface::Get()->GetRayTracingSupport(), descriptor);

            // register the ray tracing and global pipeline state object with the dispatch-item
            m_dispatchRaysItem.SetRayTracingPipelineState(m_rayTracingPipelineState.get());
            m_dispatchRaysItem.SetPipelineState(m_globalPipelineState.get());

            // make sure the shader table rebuilds if we're hotreloading
            m_rayTracingShaderTableRevision = 0;

            // store the max ray length
            m_maxRayLength = m_passData->m_maxRayLength;

            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_rayGenerationShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_closestHitShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_closestHitProceduralShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_missShaderAssetReference.m_assetId);
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(m_passData->m_intersectionShaderAssetReference.m_assetId);
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

        void RayTracingPass::BuildInternal()
        {
            if (m_indirectDispatch)
            {
                if (!m_indirectDispatchRaysBufferSignature)
                {
                    AZ::RHI::IndirectBufferLayout bufferLayout;
                    bufferLayout.AddIndirectCommand(AZ::RHI::IndirectCommandDescriptor(AZ::RHI::IndirectCommandType::DispatchRays));

                    if (!bufferLayout.Finalize())
                    {
                        AZ_Assert(false, "Fail to finalize Indirect Layout");
                    }

                    m_indirectDispatchRaysBufferSignature = aznew AZ::RHI::IndirectBufferSignature();
                    AZ::RHI::IndirectBufferSignatureDescriptor signatureDescriptor{};
                    signatureDescriptor.m_layout = bufferLayout;
                    [[maybe_unused]] auto result = m_indirectDispatchRaysBufferSignature->Init(
                        AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), signatureDescriptor);

                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Fail to initialize Indirect Buffer Signature");
                }

                m_indirectDispatchRaysBufferBinding = nullptr;
                if (!m_indirectDispatchBufferSlotName.IsEmpty())
                {
                    m_indirectDispatchRaysBufferBinding = FindAttachmentBinding(m_indirectDispatchBufferSlotName);
                    AZ_Assert(m_indirectDispatchRaysBufferBinding,
                            "[RaytracingPass '%s']: Indirect dispatch buffer slot %s not found.",
                            GetPathName().GetCStr(),
                            m_indirectDispatchBufferSlotName.GetCStr());
                    if (m_indirectDispatchRaysBufferBinding)
                    {
                        AZ_Assert(
                            m_indirectDispatchRaysBufferBinding->m_scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Indirect,
                            "[RaytracingPass '%s']: Indirect dispatch buffer slot %s needs ScopeAttachmentUsage::Indirect.",
                            GetPathName().GetCStr(),
                            m_indirectDispatchBufferSlotName.GetCStr())
                    }
                }
                else
                {
                    for (auto& binding : m_attachmentBindings)
                    {
                        if (binding.m_scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Indirect)
                        {
                            m_indirectDispatchRaysBufferBinding = &binding;
                            break;
                        }
                    }
                    AZ_Assert(m_indirectDispatchRaysBufferBinding,
                    "[RaytracingPass '%s']: No valid indirect dispatch buffer slot found.",
                    GetPathName().GetCStr());
                }



                if (!m_dispatchRaysIndirectBuffer)
                {
                    m_dispatchRaysIndirectBuffer =
                        aznew AZ::RHI::DispatchRaysIndirectBuffer{ AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() };
                    m_dispatchRaysIndirectBuffer->Init(
                        AZ::RPI::BufferSystemInterface::Get()->GetCommonBufferPool(AZ::RPI::CommonBufferPoolType::Indirect).get());
                }
            }
            else if (m_fullscreenDispatch)
            {
                m_fullscreenSizeSourceBinding = nullptr;
                if (!m_fullscreenSizeSourceSlotName.IsEmpty())
                {
                    m_fullscreenSizeSourceBinding = FindAttachmentBinding(m_fullscreenSizeSourceSlotName);
                    AZ_Assert(
                            m_fullscreenSizeSourceBinding,
                            "[RaytracingPass '%s']: Fullscreen size source slot %s not found.",
                            GetPathName().GetCStr(),
                            m_fullscreenSizeSourceSlotName.GetCStr());
                }
                else
                {
                    if (GetOutputCount() > 0)
                    {
                        m_fullscreenSizeSourceBinding = &GetOutputBinding(0);
                    }
                    else if (!m_fullscreenSizeSourceBinding && GetInputOutputCount() > 0)
                    {
                        m_fullscreenSizeSourceBinding = &GetInputOutputBinding(0);
                    }
                    AZ_Assert(
                        m_fullscreenSizeSourceBinding,
                        "[RaytracingPass '%s']: No valid Output or InputOutput slot as a fullscreen size source found.",
                        GetPathName().GetCStr());
                }
            }
        }

        void RayTracingPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (!rayTracingFeatureProcessor)
            {
                return;
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
                    RHI::BufferViewDescriptor tlasBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = tlasAttachmentId;
                    desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::RayTracingShader);
                }
            }
        }

        void RayTracingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            if (m_indirectDispatch)
            {
                if (m_indirectDispatchRaysBufferBinding)
                {
                    auto& attachment{ m_indirectDispatchRaysBufferBinding->GetAttachment() };
                    AZ_Assert(
                        attachment,
                        "[RayTracingPass '%s']: Indirect dispatch buffer slot %s has no attachment.",
                        GetPathName().GetCStr(),
                        m_indirectDispatchRaysBufferBinding->m_name.GetCStr());

                    if (attachment)
                    {
                        auto* indirectDispatchBuffer{ context.GetBuffer(attachment->GetAttachmentId()) };
                        m_indirectDispatchRaysBufferView = AZ::RHI::IndirectBufferView{ *indirectDispatchBuffer,
                                                                                        *m_indirectDispatchRaysBufferSignature,
                                                                                        0,
                                                                                        sizeof(DispatchRaysIndirectCommand),
                                                                                        sizeof(DispatchRaysIndirectCommand) };

                        RHI::DispatchRaysIndirect dispatchRaysArgs(
                            1, m_indirectDispatchRaysBufferView, 0, m_dispatchRaysIndirectBuffer.get());

                        m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
                    }
                }
            }
            else if (m_fullscreenDispatch)
            {
                auto& attachment = m_fullscreenSizeSourceBinding->GetAttachment();
                AZ_Assert(
                    attachment,
                    "[RaytracingPass '%s']: Slot %s has no attachment for fullscreen size source.",
                    GetPathName().GetCStr(),
                    m_fullscreenSizeSourceBinding->m_name.GetCStr());
                AZ::RHI::DispatchRaysDirect dispatchRaysArgs;
                if (attachment)
                {
                    AZ_Assert(
                        attachment->GetAttachmentType() == AZ::RHI::AttachmentType::Image,
                        "[RaytracingPass '%s']: Slot %s must be an image for fullscreen size source.",
                        GetPathName().GetCStr(),
                        m_fullscreenSizeSourceBinding->m_name.GetCStr());

                    auto imageDescriptor = context.GetImageDescriptor(attachment->GetAttachmentId());
                    dispatchRaysArgs.m_width = imageDescriptor.m_size.m_width;
                    dispatchRaysArgs.m_height = imageDescriptor.m_size.m_height;
                    dispatchRaysArgs.m_depth = imageDescriptor.m_size.m_depth;
                }
                m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
            }
            else
            {
                AZ::RHI::DispatchRaysDirect dispatchRaysArgs{ m_passData->m_threadCountX,
                                                              m_passData->m_threadCountY,
                                                              m_passData->m_threadCountZ };
                m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
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

            if (!m_rayTracingShaderTable || m_rayTracingShaderTableRevision != rayTracingFeatureProcessor->GetRevision())
            {
                // scene changed, need to rebuild the shader table
                m_rayTracingShaderTableRevision = rayTracingFeatureProcessor->GetRevision();
                m_rayTracingShaderTable = aznew AZ::RHI::RayTracingShaderTable();
                m_rayTracingShaderTable->Init(
                    AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), rayTracingFeatureProcessor->GetBufferPools());

                AZStd::shared_ptr<RHI::RayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<RHI::RayTracingShaderTableDescriptor>();

                if (rayTracingFeatureProcessor->HasGeometry())
                {
                    // build the ray tracing shader table descriptor
                    RHI::RayTracingShaderTableDescriptor* descriptorBuild = descriptor->Build(AZ::Name("RayTracingShaderTable"), m_rayTracingPipelineState)
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

                // register the shader-table with the dispatch item
                m_dispatchRaysItem.SetRayTracingPipelineState(m_rayTracingPipelineState.get());
                m_dispatchRaysItem.SetRayTracingShaderTable(m_rayTracingShaderTable.get());
            }

            // Collect and register the Srgs (RayTracingGlobal, RayTracingScene, ViewSrg, SceneSrg and RayTracingMaterialSrg)
            // The more consistent way would be to call BindSrg() of the RenderPass, and then call
            // SetSrgsForDispatchRays() in BuildCommandListInternal, but that function doesn't exist.
            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderResourceGroup->SetConstant(m_maxRayLengthInputIndex, m_maxRayLength);
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
                m_rayTracingSRGsToBind.push_back(m_shaderResourceGroup->GetRHIShaderResourceGroup());
            }

            if (m_requiresRayTracingSceneSrg)
            {
                m_rayTracingSRGsToBind.push_back(rayTracingFeatureProcessor->GetRayTracingSceneSrg()->GetRHIShaderResourceGroup());
            }

            if (m_requiresViewSrg)
            {
                RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
                if (view)
                {
                    m_rayTracingSRGsToBind.push_back(view->GetShaderResourceGroup()->GetRHIShaderResourceGroup());
                }
            }

            if (m_requiresSceneSrg)
            {
                m_rayTracingSRGsToBind.push_back(scene->GetShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            if (m_requiresRayTracingMaterialSrg)
            {
                m_rayTracingSRGsToBind.push_back(rayTracingFeatureProcessor->GetRayTracingMaterialSrg()->GetRHIShaderResourceGroup());
            }
        }

        void RayTracingPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor || !rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() ||
                !rayTracingFeatureProcessor->HasGeometry() || !m_rayTracingShaderTable)
            {
                return;
            }

            if (m_dispatchRaysShaderTableRevision != m_rayTracingShaderTableRevision)
            {
                m_dispatchRaysShaderTableRevision = m_rayTracingShaderTableRevision;
                if (m_dispatchRaysIndirectBuffer)
                {
                    m_dispatchRaysIndirectBuffer->Build(m_rayTracingShaderTable.get());
                }
            }

            // TODO: change this to BindSrgsForDispatchRays() as soon as it exists
            // IMPORTANT: The data in shaderResourceGroups must be sorted by (entry)->GetBindingSlot() (FrequencyId value in SRG source file
            // from SrgSemantics.azsli) in order for them to be correctly assigned by Vulkan
            AZStd::sort(
                m_rayTracingSRGsToBind.begin(),
                m_rayTracingSRGsToBind.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    return lhs->GetBindingSlot() < rhs->GetBindingSlot();
                });
            m_dispatchRaysItem.SetShaderResourceGroups(m_rayTracingSRGsToBind.data(), static_cast<uint32_t>(m_rayTracingSRGsToBind.size()));

            // submit the DispatchRays item
            context.GetCommandList()->Submit(m_dispatchRaysItem.GetDeviceDispatchRaysItem(context.GetDeviceIndex()));
        }

        void RayTracingPass::FrameEndInternal()
        {
            m_rayTracingSRGsToBind.clear();
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
