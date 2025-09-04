/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingMaterialPass.h>
#include <Atom/Feature/RayTracing/RayTracingMaterialPassData.h>
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
        RPI::Ptr<RayTracingMaterialPass> RayTracingMaterialPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RayTracingMaterialPass> pass = aznew RayTracingMaterialPass(descriptor);
            return pass;
        }

        RayTracingMaterialPass::RayTracingMaterialPass(const RPI::PassDescriptor& descriptor)
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

            m_passData = RPI::PassUtils::GetPassData<RayTracingMaterialPassData>(m_passDescriptor);
            if (m_passData == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingMaterialPass [%s]: Invalid RayTracingMaterialPassData", GetPathName().GetCStr());
                return;
            }

            m_indirectDispatch = m_passData->m_indirectDispatch;
            m_indirectDispatchBufferSlotName = m_passData->m_indirectDispatchBufferSlotName;

            m_fullscreenDispatch = m_passData->m_fullscreenDispatch;
            m_fullscreenSizeSourceSlotName = m_passData->m_fullscreenSizeSourceSlotName;

            AZ_Assert(
                !(m_indirectDispatch && m_fullscreenDispatch),
                "[RayTracingMaterialPass '%s']: Only one of the dispatch options (indirect, fullscreen) can be active",
                GetPathName().GetCStr());

            m_defaultShaderAttachmentStage = RHI::ScopeAttachmentStage::RayTracingShader;

            SetDrawListTag(m_passData->m_drawListTag);
        }

        RayTracingMaterialPass::~RayTracingMaterialPass()
        {
        }

        void RayTracingMaterialPass::SetDrawListTag(Name drawListName)
        {
            // Use AcquireTag to register a draw list tag if it doesn't exist.
            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
            m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(drawListName);
            m_flags.m_hasDrawListTag = true;
        }

        void RayTracingMaterialPass::UpdateShaderLibraries()
        {
            m_maxRayLengthInputIndex.Reset();

            RayTracingFeatureProcessor* rayTracingFeatureProcessor =
                GetScene() ? GetScene()->GetFeatureProcessor<RayTracingFeatureProcessor>() : nullptr;

            if (rayTracingFeatureProcessor && m_rayTracingShaderTableRevision != rayTracingFeatureProcessor->GetRevision())
            {
                const auto& shaderManager = rayTracingFeatureProcessor->GetMaterialShaderManager();
                if (shaderManager.HasShaderLibraries(m_drawListTag))
                {
                    auto& shaderLibraries = shaderManager.GetShaderLibraries(m_drawListTag);

                    m_shaderResourceGroup = shaderLibraries->CreatePassSrg();
                    RPI::PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());

                    m_requiresViewSrg = shaderLibraries->RequiresSrg(RPI::SrgBindingSlot::View);
                    m_requiresSceneSrg = shaderLibraries->RequiresSrg(RPI::SrgBindingSlot::Scene);
                    m_requiresMaterialSrg = shaderLibraries->RequiresSrg(RPI::SrgBindingSlot::Material);
                    m_requiresRayTracingSceneSrg = shaderLibraries->RequiresSrg(RayTracingSceneSrgBindingSlot);

                    // register the ray tracing and global pipeline state object with the dispatch-item
                    m_dispatchRaysItem.SetPipelineState(shaderLibraries->GetGlobalPipelineState());
                    m_dispatchRaysItem.SetRayTracingPipelineState(shaderLibraries->GetRayTracingPipelineState());
                    m_dispatchRaysItem.SetRayTracingShaderTable(shaderLibraries->GetRayTracingShaderTable());
                    // store the max ray length
                    m_maxRayLength = m_passData->m_maxRayLength;
                }
                m_rayTracingShaderTableRevision = rayTracingFeatureProcessor->GetRevision();
            }
        }

        bool RayTracingMaterialPass::IsEnabled() const
        {
            if (!RenderPass::IsEnabled())
            {
                return false;
            }

            if (m_pipeline == nullptr)
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

        void RayTracingMaterialPass::BuildInternal()
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
                    AZ_Assert(
                        m_indirectDispatchRaysBufferBinding,
                        "[RayTracingMaterialPass '%s']: Indirect dispatch buffer slot %s not found.",
                        GetPathName().GetCStr(),
                        m_indirectDispatchBufferSlotName.GetCStr());
                    if (m_indirectDispatchRaysBufferBinding)
                    {
                        AZ_Assert(
                            m_indirectDispatchRaysBufferBinding->m_scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Indirect,
                            "[RayTracingMaterialPass '%s']: Indirect dispatch buffer slot %s needs ScopeAttachmentUsage::Indirect.",
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
                    AZ_Assert(
                        m_indirectDispatchRaysBufferBinding,
                        "[RayTracingMaterialPass '%s']: No valid indirect dispatch buffer slot found.",
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
                        "[RayTracingMaterialPass '%s']: Fullscreen size source slot %s not found.",
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
                        "[RayTracingMaterialPass '%s']: No valid Output or InputOutput slot as a fullscreen size source found.",
                        GetPathName().GetCStr());
                }
            }
        }

        void RayTracingMaterialPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (!rayTracingFeatureProcessor)
            {
                return;
            }

            RPI::RenderPass::FrameBeginInternal(params);
        }

        void RayTracingMaterialPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingMaterialPass requires the RayTracingFeatureProcessor");

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
                        [[maybe_unused]] RHI::ResultCode result =
                            frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, rayTracingTlasBuffer);
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);
                    }

                    uint32_t tlasBufferByteCount =
                        aznumeric_cast<uint32_t>(rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer()->GetDescriptor().m_byteCount);
                    RHI::BufferViewDescriptor tlasBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = tlasAttachmentId;
                    desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::RayTracingShader);
                }
            }
        }

        void RayTracingMaterialPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingMaterialPass requires the RayTracingFeatureProcessor");

            if (m_indirectDispatch)
            {
                if (m_indirectDispatchRaysBufferBinding)
                {
                    auto& attachment{ m_indirectDispatchRaysBufferBinding->GetAttachment() };
                    AZ_Assert(
                        attachment,
                        "[RayTracingMaterialPass '%s']: Indirect dispatch buffer slot %s has no attachment.",
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
                    "[RayTracingMaterialPass '%s']: Slot %s has no attachment for fullscreen size source.",
                    GetPathName().GetCStr(),
                    m_fullscreenSizeSourceBinding->m_name.GetCStr());
                AZ::RHI::DispatchRaysDirect dispatchRaysArgs;
                if (attachment)
                {
                    AZ_Assert(
                        attachment->GetAttachmentType() == AZ::RHI::AttachmentType::Image,
                        "[RayTracingMaterialPass '%s']: Slot %s must be an image for fullscreen size source.",
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

            UpdateShaderLibraries();

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

            if (m_requiresMaterialSrg)
            {
                auto* materialInstanceHandler = RPI::MaterialInstanceHandlerInterface::Get();
                auto sceneMaterialSrg = materialInstanceHandler->GetSceneMaterialSrg();
                if (sceneMaterialSrg)
                {
                    m_rayTracingSRGsToBind.push_back(sceneMaterialSrg->GetRHIShaderResourceGroup());
                }
            }
        }

        void RayTracingMaterialPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingMaterialPass requires the RayTracingFeatureProcessor");
            AZ_Assert(
                RHI::CheckBit(rayTracingFeatureProcessor->GetDeviceMask(), context.GetDeviceIndex()),
                "RayTracingMaterialPass cannot run on a device without a RayTracingAccelerationStructurePass");

            if (!rayTracingFeatureProcessor || !rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() ||
                !rayTracingFeatureProcessor->HasGeometry())
            {
                return;
            }

            // if (m_dispatchRaysShaderTableRevision != m_rayTracingShaderTableRevision)
            // {
            //     m_dispatchRaysShaderTableRevision = m_rayTracingShaderTableRevision;
            //     if (m_dispatchRaysIndirectBuffer)
            //     {
            //         m_dispatchRaysIndirectBuffer->Build(m_rayTracingShaderTable.get());
            //     }
            // }

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

        void RayTracingMaterialPass::FrameEndInternal()
        {
            m_rayTracingSRGsToBind.clear();
        }
    } // namespace Render
} // namespace AZ
