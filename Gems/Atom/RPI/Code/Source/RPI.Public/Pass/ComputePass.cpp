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
            const ComputePassData* passData = PassUtils::GetPassData<ComputePassData>(m_passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error(
                    "PassSystem", false, "[ComputePass '%s']: Trying to construct without valid ComputePassData!", GetPathName().GetCStr());
                return;
            }

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

            m_isFullscreenPass = passData->m_makeFullscreenPass;

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
        }

        void ComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(context);

            commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }

        void ComputePass::MatchDimensionsToOutput()
        {
            PassAttachment* outputAttachment = nullptr;

            if (GetOutputCount() > 0)
            {
                outputAttachment = GetOutputBinding(0).GetAttachment().get();
            }
            else if (GetInputOutputCount() > 0)
            {
                outputAttachment = GetInputOutputBinding(0).GetAttachment().get();
            }

            AZ_Assert(outputAttachment != nullptr, "[ComputePass '%s']: A fullscreen compute pass must have a valid output or input/output.",
                GetPathName().GetCStr());

            AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image,
                "[ComputePass '%s']: The output of a fullscreen compute pass must be an image.",
                GetPathName().GetCStr());

            RHI::Size targetImageSize = outputAttachment->m_descriptor.m_image.m_size;

            SetTargetThreadCounts(targetImageSize.m_width, targetImageSize.m_height, targetImageSize.m_depth);
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

        void ComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            if (m_isFullscreenPass)
            {
                MatchDimensionsToOutput();
            }

            RenderPass::FrameBeginInternal(params);
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
