/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Shadows/FullscreenShadowPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<FullscreenShadowPass> FullscreenShadowPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<FullscreenShadowPass> pass = aznew FullscreenShadowPass(descriptor);
            return pass;
        }

        FullscreenShadowPass::FullscreenShadowPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void FullscreenShadowPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            SetConstantData();
            ComputePass::CompileResources(context);
        }

        void FullscreenShadowPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            // Dispatch one compute shader thread per depth buffer pixel. These threads are divided into thread-groups that analyze one tile. (Typically 16x16 pixel tiles)
            RHI::CommandList* commandList = context.GetCommandList();
            SetSrgsForDispatch(commandList);

            RHI::Size resolution = GetDepthBufferDimensions();

            const auto test = GetOutputBinding(0);
            AZ_UNUSED(test);

            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = resolution.m_width;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = resolution.m_height;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;
            commandList->Submit(m_dispatchItem);
        }

        AZ::RHI::Size FullscreenShadowPass::GetDepthBufferDimensions()
        {
            //for (unsigned int i = 0; i < GetInputCount() ; ++i)
            //{
            //    auto b = GetInputBinding(i);
            //    auto n = b.m_name;
            //    const char* c = n.GetCStr();
            //    AZ_Printf("EXCUSE ME", "Input %s\n", c);
            //}

            //for (unsigned int i = 0; i < GetInputOutputCount(); ++i)
            //{
            //    auto b = GetInputOutputBinding(i);
            //    auto n = b.m_name;
            //    const char* c = n.GetCStr();
            //    AZ_Printf("EXCUSE ME", "InotuputOutp %s\n", c);
            //}

            //for (unsigned int i = 0; i < GetOutputCount(); ++i)
            //{
            //    auto b = GetOutputBinding(i);
            //    auto n = b.m_name;
            //    const char* c = n.GetCStr();
            //    AZ_Printf("EXCUSE ME", "output %s\n", c);
            //}
            AZ_Assert(GetInputOutputCount() == 1, "Unexpected GetInputOutputCount(). Expecting 1, got %d", GetInputOutputCount());

            AZ_Assert(
                GetInputOutputBinding(0).m_name == AZ::Name("SwapChainOutput"),
                "FullscreenShadowPass: Expecting slot 0 to be the SwapChainOutput");


            const auto swapChain = GetInputOutputBinding(0);
            const RPI::PassAttachment* attachment = swapChain.m_attachment.get();
            return attachment->m_descriptor.m_image.m_size;
        }

        void FullscreenShadowPass::ChooseShaderVariant()
        {
            const AZ::RPI::ShaderVariant& shaderVariant = CreateShaderVariant();
            CreatePipelineStateFromShaderVariant(shaderVariant);
        }

        AZ::RPI::ShaderOptionGroup FullscreenShadowPass::CreateShaderOptionGroup()
        {
            RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
            shaderOptionGroup.SetUnspecifiedToDefaultValues();
            return shaderOptionGroup;
        }

        void FullscreenShadowPass::CreatePipelineStateFromShaderVariant(const RPI::ShaderVariant& shaderVariant)
        {
            AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            m_msaaPipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
            AZ_Error("LightCulling", m_msaaPipelineState, "Failed to acquire pipeline state for shader");
        }

        const AZ::RPI::ShaderVariant& FullscreenShadowPass::CreateShaderVariant()
        {
            RPI::ShaderOptionGroup shaderOptionGroup = CreateShaderOptionGroup();
            const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(shaderOptionGroup.GetShaderVariantId());

            //Set the fallbackkey
            if (m_drawSrg)
            {
                m_drawSrg->SetShaderVariantKeyFallbackValue(shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }
            return shaderVariant;
        }

        void FullscreenShadowPass::SetConstantData()
        {
            //struct ConstantData
            //{
            //    AZStd::array<float, 2> m_unprojectZ;
            //    uint32_t depthBufferWidth;
            //    uint32_t depthBufferHeight;
            //} constantData;

            //const RHI::Size resolution = GetDepthBufferDimensions();
            //constantData.m_unprojectZ = ComputeUnprojectConstants();
            //constantData.depthBufferWidth = resolution.m_width;
            //constantData.depthBufferHeight = resolution.m_height;

            //[[maybe_unused]] bool setOk = m_shaderResourceGroup->SetConstant(m_constantDataIndex, constantData);
            //AZ_Assert(setOk, "FullscreenShadowPass::SetConstantData() - could not set constant data");
        }

        void FullscreenShadowPass::BuildInternal()
        {
            m_shader;
        }

        void FullscreenShadowPass::OnShaderReloaded()
        {
            LoadShader();
            AZ_Assert(GetPassState() != RPI::PassState::Rendering, "FullscreenShadowPass: Trying to reload shader during rendering");
            if (GetPassState() == RPI::PassState::Idle)
            {
            }
        }

        void FullscreenShadowPass::OnShaderReinitialized(const AZ::RPI::Shader&)
        {
            OnShaderReloaded();
        }

        void FullscreenShadowPass::OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>&)
        {
            OnShaderReloaded();
        }

        void FullscreenShadowPass::OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant&)
        {
            OnShaderReloaded();
        }

    }   // namespace Render
}   // namespace AZ
