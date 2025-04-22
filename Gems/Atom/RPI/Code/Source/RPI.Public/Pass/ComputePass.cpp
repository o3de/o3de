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
#include <Atom/RHI/DevicePipelineState.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

using uint = uint32_t;
using uint4 = uint[4];
#include "../../../Feature/Common/Assets/ShaderLib/Atom/Features/IndirectRendering.azsli"

namespace AZ
{
    namespace RPI
    {
        ComputePass::~ComputePass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        Ptr<ComputePass> ComputePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<ComputePass> pass = aznew ComputePass(descriptor);
            return pass;
        }

        ComputePass::ComputePass(const PassDescriptor& descriptor, AZ::Name supervariant)
            : RenderPass(descriptor)
            , m_dispatchItem(RHI::MultiDevice::AllDevices)
            , m_passDescriptor(descriptor)
        {
            m_flags.m_canBecomeASubpass = false;
            const ComputePassData* passData = PassUtils::GetPassData<ComputePassData>(m_passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error(
                    "PassSystem", false, "[ComputePass '%s']: Trying to construct without valid ComputePassData!", GetPathName().GetCStr());
                return;
            }

            m_indirectDispatch = passData->m_indirectDispatch;
            m_indirectDispatchBufferSlotName = passData->m_indirectDispatchBufferSlotName;

            m_fullscreenDispatch = passData->m_fullscreenDispatch;
            m_fullscreenSizeSourceSlotName = passData->m_fullscreenSizeSourceSlotName;

            AZ_Assert(
                !(m_indirectDispatch && m_fullscreenDispatch),
                "[ComputePass '%s']: Only one of the dispatch options (indirect, fullscreen) can be active.",
                GetPathName().GetCStr());

            RHI::DispatchDirect dispatchArgs;
            dispatchArgs.m_totalNumberOfThreadsX = passData->m_totalNumberOfThreadsX;
            dispatchArgs.m_totalNumberOfThreadsY = passData->m_totalNumberOfThreadsY;
            dispatchArgs.m_totalNumberOfThreadsZ = passData->m_totalNumberOfThreadsZ;
            m_dispatchItem.SetArguments(dispatchArgs);

            LoadShader(supervariant);

            m_defaultShaderAttachmentStage = RHI::ScopeAttachmentStage::ComputeShader;
        }

        void ComputePass::LoadShader(AZ::Name supervariant)
        {
            // Load ComputePassData...
            const ComputePassData* passData = PassUtils::GetPassData<ComputePassData>(m_passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Trying to construct without valid ComputePassData!",
                    GetPathName().GetCStr());
                return;
            }

            // Hardware Queue Class
            if (passData->m_useAsyncCompute)
            {
                m_hardwareQueueClass = RHI::HardwareQueueClass::Compute;
            }

            // Load Shader
            Data::Asset<ShaderAsset> shaderAsset;
            if (passData->m_shaderReference.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(passData->m_shaderReference.m_assetId, passData->m_shaderReference.m_filePath);
            }

            if (!shaderAsset.IsReady())
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset, supervariant);
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Failed to create shader instance from asset '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            // Load Pass SRG...
            const auto passSrgLayout = m_shader->FindShaderResourceGroupLayout(SrgBindingSlot::Pass);
            if (passSrgLayout)
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(shaderAsset, m_shader->GetSupervariantIndex(), passSrgLayout->GetName());

                AZ_Assert(m_shaderResourceGroup, "[ComputePass '%s']: Failed to create SRG from shader asset '%s'",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());

                PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());
            }

            // Load Draw SRG...
            const bool compileDrawSrg = false; // The SRG will be compiled in CompileResources()
            m_drawSrg = m_shader->CreateDefaultDrawSrg(compileDrawSrg);

            if (m_dispatchItem.GetArguments().m_type == RHI::DispatchType::Direct)
            {
                auto arguments = m_dispatchItem.GetArguments();
                const auto outcome = RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), arguments.m_direct);
                if (!outcome.IsSuccess())
                {
                    AZ_Error(
                        "PassSystem",
                        false,
                        "[ComputePass '%s']: Shader '%.*s' contains invalid numthreads arguments:\n%s",
                        GetPathName().GetCStr(),
                        passData->m_shaderReference.m_filePath.size(),
                        passData->m_shaderReference.m_filePath.data(),
                        outcome.GetError().c_str());
                }
                m_dispatchItem.SetArguments(arguments);
            }

            // Setup pipeline state...
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            ShaderOptionGroup options = m_shader->GetDefaultShaderOptions();
            m_shader->GetDefaultVariant().ConfigurePipelineState(pipelineStateDescriptor, options);

            m_dispatchItem.SetPipelineState(m_shader->AcquirePipelineState(pipelineStateDescriptor));
            if (m_drawSrg && m_shader->GetDefaultVariant().UseKeyFallback())
            {
                m_drawSrg->SetShaderVariantKeyFallbackValue(options.GetShaderVariantKeyFallbackValue());
            }

            OnShaderReloadedInternal();

            ShaderReloadNotificationBus::Handler::BusDisconnect();
            ShaderReloadNotificationBus::Handler::BusConnect(passData->m_shaderReference.m_assetId);
        }

        // Scope producer functions

        void ComputePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
            }
            if (m_drawSrg != nullptr)
            {
                BindSrg(m_drawSrg->GetRHIShaderResourceGroup());
                m_drawSrg->Compile();
            }

            if (m_indirectDispatch && m_indirectDispatchBufferBinding)
            {
                auto& attachment = m_indirectDispatchBufferBinding->GetAttachment();
                AZ_Assert(
                    attachment,
                    "[ComputePass '%s']: Indirect dispatch buffer slot %s has no attachment.",
                    GetPathName().GetCStr(),
                    m_indirectDispatchBufferBinding->m_name.GetCStr());

                if (attachment)
                {
                    auto buffer = context.GetBuffer(attachment->GetAttachmentId());
                    AZ_Assert(
                        buffer,
                        "[ComputePass '%s']: Attachment connected to Indirect dispatch buffer slot %s has no buffer",
                        GetPathName().GetCStr(),
                        m_indirectDispatchBufferBinding->m_name.GetCStr());
                    m_indirectDispatchBufferView = {
                        *buffer, *m_indirectDispatchBufferSignature, 0, sizeof(DispatchIndirectCommand), sizeof(DispatchIndirectCommand)
                    };

                    AZ::RHI::DispatchIndirect dispatchArgs(1, m_indirectDispatchBufferView, 0);
                    m_dispatchItem.SetArguments(dispatchArgs);
                }
            }
            else if (m_fullscreenDispatch && m_fullscreenSizeSourceBinding)
            {
                auto& attachment = m_fullscreenSizeSourceBinding->GetAttachment();
                AZ_Assert(
                    attachment,
                    "[ComputePass '%s']: Slot %s has no attachment for fullscreen size source.",
                    GetPathName().GetCStr(),
                    m_fullscreenSizeSourceBinding->m_name.GetCStr());
                if (attachment)
                {
                    AZ_Assert(
                        attachment->GetAttachmentType() == AZ::RHI::AttachmentType::Image,
                        "[ComputePass '%s']: Slot %s must be an image for fullscreen size source.",
                        GetPathName().GetCStr(),
                        m_fullscreenSizeSourceBinding->m_name.GetCStr());

                    auto imageDescriptor = context.GetImageDescriptor(attachment->GetAttachmentId());
                    // We are using the ArraySize or the image depth, whichever is bigger.
                    // Note that this will fail for an array of 3d textures.
                    auto depth = AZStd::max(imageDescriptor.m_size.m_depth, static_cast<uint32_t>(imageDescriptor.m_arraySize));
                    SetTargetThreadCounts(imageDescriptor.m_size.m_width, imageDescriptor.m_size.m_height, depth);
                }
            }
        }

        void ComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(context);

            commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }

        void ComputePass::SetTargetThreadCounts(uint32_t targetThreadCountX, uint32_t targetThreadCountY, uint32_t targetThreadCountZ)
        {
            auto arguments{m_dispatchItem.GetArguments()};
            arguments.m_direct.m_totalNumberOfThreadsX = targetThreadCountX;
            arguments.m_direct.m_totalNumberOfThreadsY = targetThreadCountY;
            arguments.m_direct.m_totalNumberOfThreadsZ = targetThreadCountZ;
            m_dispatchItem.SetArguments(arguments);
        }

        Data::Instance<ShaderResourceGroup> ComputePass::GetShaderResourceGroup() const
        {
            return m_shaderResourceGroup;
        }

        Data::Instance<Shader> ComputePass::GetShader() const
        {
            return m_shader;
        }

        void ComputePass::BuildInternal()
        {
            RenderPass::BuildInternal();

            if (m_indirectDispatch)
            {
                m_indirectDispatchBufferBinding = nullptr;
                if (!m_indirectDispatchBufferSlotName.IsEmpty())
                {
                    m_indirectDispatchBufferBinding = FindAttachmentBinding(m_indirectDispatchBufferSlotName);
                    AZ_Assert(m_indirectDispatchBufferBinding,
                            "[ComputePass '%s']: Indirect dispatch buffer slot %s not found.",
                            GetPathName().GetCStr(),
                            m_indirectDispatchBufferSlotName.GetCStr());
                    if (m_indirectDispatchBufferBinding)
                    {
                        AZ_Assert(
                            m_indirectDispatchBufferBinding->m_scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Indirect,
                            "[ComputePass '%s']: Indirect dispatch buffer slot %s needs ScopeAttachmentUsage::Indirect.",
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
                            m_indirectDispatchBufferBinding = &binding;
                            break;
                        }
                    }
                    AZ_Assert(
                        m_indirectDispatchBufferBinding,
                        "[ComputePass '%s']: No valid indirect dispatch buffer slot found.",
                        GetPathName().GetCStr());
                }

                AZ::RHI::IndirectBufferLayout indirectDispatchBufferLayout;
                indirectDispatchBufferLayout.AddIndirectCommand(AZ::RHI::IndirectCommandDescriptor(AZ::RHI::IndirectCommandType::Dispatch));

                if (!indirectDispatchBufferLayout.Finalize())
                {
                    AZ_Assert(false, "[ComputePass '%s']: Failed to finalize Indirect Layout", GetPathName().GetCStr());
                }

                m_indirectDispatchBufferSignature = aznew AZ::RHI::IndirectBufferSignature;
                AZ::RHI::IndirectBufferSignatureDescriptor signatureDescriptor{};
                signatureDescriptor.m_layout = indirectDispatchBufferLayout;
                [[maybe_unused]] auto result =
                    m_indirectDispatchBufferSignature->Init(AZ::RHI::MultiDevice::AllDevices, signatureDescriptor);

                AZ_Assert(
                    result == AZ::RHI::ResultCode::Success,
                    "[ComputePass '%s']: Failed to initialize Indirect Buffer Signature",
                    GetPathName().GetCStr());
            }
            else if (m_fullscreenDispatch)
            {
                m_fullscreenSizeSourceBinding = nullptr;
                if (!m_fullscreenSizeSourceSlotName.IsEmpty())
                {
                    m_fullscreenSizeSourceBinding = FindAttachmentBinding(m_fullscreenSizeSourceSlotName);
                    AZ_Assert(
                        m_fullscreenSizeSourceBinding,
                        "[ComputePass '%s']: Fullscreen size source slot %s not found.",
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
                        "[ComputePass '%s']: No valid Output or InputOutput slot as a fullscreen size source found.",
                        GetPathName().GetCStr());
                }
            }
        }

        void ComputePass::OnShaderReinitialized(const Shader& shader)
        {
            AZ_UNUSED(shader);
            LoadShader();
        }

        void ComputePass::OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset)
        {
            AZ_UNUSED(shaderAsset);
            LoadShader();
        }

        void ComputePass::OnShaderVariantReinitialized(const ShaderVariant&)
        {
            LoadShader();
        }

        void ComputePass::SetComputeShaderReloadedCallback(ComputeShaderReloadedCallback callback)
        {
            m_shaderReloadedCallback = callback;
        }

        void ComputePass::UpdateShaderOptions(const ShaderVariantId& shaderVariantId)
        {
            const ShaderVariant& shaderVariant = m_shader->GetVariant(shaderVariantId);
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shaderVariantId);

            m_dispatchItem.SetPipelineState(m_shader->AcquirePipelineState(pipelineStateDescriptor));
            if (m_drawSrg && shaderVariant.UseKeyFallback())
            {
                m_drawSrg->SetShaderVariantKeyFallbackValue(shaderVariantId.m_key);
            }
        }

        void ComputePass::OnShaderReloadedInternal()
        {
            if (m_shaderReloadedCallback)
            {
                m_shaderReloadedCallback(this);
            }
        }

    }   // namespace RPI
}   // namespace AZ
