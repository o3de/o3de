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
#include <Atom/RHI/PipelineState.h>

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

        ComputePass::ComputePass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
            , m_passDescriptor(descriptor)
        {
            LoadShader();
        }

        void ComputePass::LoadShader()
        {
            // Load ComputePassData...
            const ComputePassData* passData = PassUtils::GetPassData<ComputePassData>(m_passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Trying to construct without valid ComputePassData!",
                    GetPathName().GetCStr());
                return;
            }

            // Load Shader
            Data::Asset<ShaderAsset> shaderAsset;
            if (passData->m_shaderReference.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(passData->m_shaderReference.m_assetId, passData->m_shaderReference.m_filePath);
            }

            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset);
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Failed to load shader '%s'!",
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
            const auto drawSrgLayout = m_shader->FindShaderResourceGroupLayout(SrgBindingSlot::Draw);
            if (drawSrgLayout)
            {
                m_drawSrg = ShaderResourceGroup::Create(shaderAsset, m_shader->GetSupervariantIndex(), drawSrgLayout->GetName());
            }

            RHI::DispatchDirect dispatchArgs;
            dispatchArgs.m_totalNumberOfThreadsX = passData->m_totalNumberOfThreadsX;
            dispatchArgs.m_totalNumberOfThreadsY = passData->m_totalNumberOfThreadsY;
            dispatchArgs.m_totalNumberOfThreadsZ = passData->m_totalNumberOfThreadsZ;

            const auto outcome = RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Shader '%.*s' contains invalid numthreads arguments:\n%s",
                        GetPathName().GetCStr(), passData->m_shaderReference.m_filePath.size(), passData->m_shaderReference.m_filePath.data(), outcome.GetError().c_str());
            }

            m_dispatchItem.m_arguments = dispatchArgs;

            m_isFullscreenPass = passData->m_makeFullscreenPass;

            // Setup pipeline state...
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            m_dispatchItem.m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);

            ShaderReloadNotificationBus::Handler::BusDisconnect();
            ShaderReloadNotificationBus::Handler::BusConnect(passData->m_shaderReference.m_assetId);
        }

        // Scope producer functions

        void ComputePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);
            frameGraph.SetEstimatedItemCount(1);
        }

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

            SetSrgsForDispatch(commandList);

            commandList->Submit(m_dispatchItem);
        }

        void ComputePass::MatchDimensionsToOutput()
        {
            PassAttachment* outputAttachment = nullptr;
            
            if (GetOutputCount() > 0)
            {
                outputAttachment = GetOutputBinding(0).m_attachment.get();
            }
            else if (GetInputOutputCount() > 0)
            {
                outputAttachment = GetInputOutputBinding(0).m_attachment.get();
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
            auto& arguments = m_dispatchItem.m_arguments.m_direct;
            arguments.m_totalNumberOfThreadsX = targetThreadCountX;
            arguments.m_totalNumberOfThreadsY = targetThreadCountY;
            arguments.m_totalNumberOfThreadsZ = targetThreadCountZ;
        }

        Data::Instance<ShaderResourceGroup> ComputePass::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
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

    }   // namespace RPI
}   // namespace AZ
