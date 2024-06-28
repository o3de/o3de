/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/LightCullingTilePreparePass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DevicePipelineState.h>
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
        RPI::Ptr<LightCullingTilePreparePass> LightCullingTilePreparePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LightCullingTilePreparePass> pass = aznew LightCullingTilePreparePass(descriptor);
            return pass;
        }

        LightCullingTilePreparePass::LightCullingTilePreparePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
            , m_msaaNoneName("MsaaMode::None")
            , m_msaaMode2xName("MsaaMode::Msaa2x")
            , m_msaaMode4xName("MsaaMode::Msaa4x")
            , m_msaaMode8xName("MsaaMode::Msaa8x")
            , m_msaaOptionName("o_msaaMode")
        {
        }

        void LightCullingTilePreparePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            SetConstantData();
            ComputePass::CompileResources(context);
        }

        void LightCullingTilePreparePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            // Dispatch one compute shader thread per depth buffer pixel. These threads are divided into thread-groups that analyze one tile. (Typically 16x16 pixel tiles)
            RHI::CommandList* commandList = context.GetCommandList();
            SetSrgsForDispatch(context);

            RHI::Size resolution = GetDepthBufferDimensions();

            auto arguments{m_dispatchItem.GetArguments()};
            arguments.m_direct.m_totalNumberOfThreadsX = resolution.m_width;
            arguments.m_direct.m_totalNumberOfThreadsY = resolution.m_height;
            arguments.m_direct.m_totalNumberOfThreadsZ = 1;
            m_dispatchItem.SetArguments(arguments);
            m_dispatchItem.SetPipelineState(m_msaaPipelineState.get());
            commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }

        AZ::RHI::Size LightCullingTilePreparePass::GetDepthBufferDimensions()
        {
            AZ_Assert(GetInputBinding(0).m_name == AZ::Name("Depth"), "LightCullingTilePrepare: Expecting slot 0 to be the depth buffer");

            const RPI::PassAttachment* attachment = GetInputBinding(0).GetAttachment().get();
            return attachment->m_descriptor.m_image.m_size;
        }

        AZStd::array<float, 2> LightCullingTilePreparePass::ComputeUnprojectConstants() const
        {
            AZStd::array<float, 2> unprojectConstants;
            const auto& view = m_pipeline->GetFirstView(GetPipelineViewTag());

            // Our view to clip matrix is right-hand and column major
            // i.e. something like this:
            // [- -  - -][x]
            // [- -  - -][y]
            // [0 0  A B][z]
            // [0 0 -1 0][1]
            // To unproject from depth buffer to Z, we want to pack the A and B variables into a constant buffer:
            unprojectConstants[0] = (-view->GetViewToClipMatrix().GetRow(2).GetElement(3));
            unprojectConstants[1] = (view->GetViewToClipMatrix().GetRow(2).GetElement(2));
            return unprojectConstants;
        }

        void LightCullingTilePreparePass::ChooseShaderVariant()
        {
            auto [shaderVariant, shaderOptions] = CreateShaderVariant();
            CreatePipelineStateFromShaderVariant(shaderVariant, shaderOptions);
        }

        AZ::Name LightCullingTilePreparePass::GetMultiSampleName()
        {
            const RHI::MultisampleState msaa = GetMultiSampleState();
            switch (msaa.m_samples)
            {
            case 1:
                return m_msaaNoneName;
            case 2:
                return m_msaaMode2xName;
            case 4:
                return m_msaaMode4xName;
            case 8:
                return m_msaaMode8xName;
            default:
                AZ_Error("LightCullingTilePreparePass", false, "Unhandled number of Msaa samples: %u", msaa.m_samples);
                return m_msaaNoneName;
            }
        }

        AZ::RHI::MultisampleState LightCullingTilePreparePass::GetMultiSampleState()
        {
            AZ_Assert(GetInputBinding(0).m_name == AZ::Name("Depth"), "LightCullingTilePrepare: Expecting slot 0 to be the depth buffer");

            const RPI::PassAttachment* attachment = GetInputBinding(0).GetAttachment().get();
            return attachment->m_descriptor.m_image.m_multisampleState;
        }

        AZ::RPI::ShaderOptionGroup LightCullingTilePreparePass::CreateShaderOptionGroup()
        {
            RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
            shaderOptionGroup.SetValue(m_msaaOptionName, GetMultiSampleName());
            shaderOptionGroup.SetUnspecifiedToDefaultValues();
            return shaderOptionGroup;
        }

        void LightCullingTilePreparePass::CreatePipelineStateFromShaderVariant(
            const RPI::ShaderVariant& shaderVariant, const RPI::ShaderOptionGroup& shaderOptions)
        {
            AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shaderOptions);
            m_msaaPipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
            AZ_Error("LightCulling", m_msaaPipelineState, "Failed to acquire pipeline state for shader");
        }

        AZStd::pair<const AZ::RPI::ShaderVariant&, RPI::ShaderOptionGroup> LightCullingTilePreparePass::CreateShaderVariant()
        {
            RPI::ShaderOptionGroup shaderOptionGroup = CreateShaderOptionGroup();
            const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(shaderOptionGroup.GetShaderVariantId());

            //Set the fallbackkey
            if (shaderVariant.UseKeyFallback() && m_drawSrg)
            {
                m_drawSrg->SetShaderVariantKeyFallbackValue(shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }
            return { shaderVariant, shaderOptionGroup };
        }

        void LightCullingTilePreparePass::SetConstantData()
        {
            struct ConstantData
            {
                AZStd::array<float, 2> m_unprojectZ;
                uint32_t depthBufferWidth;
                uint32_t depthBufferHeight;
            } constantData;

            const RHI::Size resolution = GetDepthBufferDimensions();
            constantData.m_unprojectZ = ComputeUnprojectConstants();
            constantData.depthBufferWidth = resolution.m_width;
            constantData.depthBufferHeight = resolution.m_height;

            [[maybe_unused]] bool setOk = m_shaderResourceGroup->SetConstant(m_constantDataIndex, constantData);
            AZ_Assert(setOk, "LightCullingTilePreparePass::SetConstantData() - could not set constant data");
        }

        void LightCullingTilePreparePass::BuildInternal()
        {
            ChooseShaderVariant();
        }

        void LightCullingTilePreparePass::OnShaderReloaded()
        {
            LoadShader();
            AZ_Assert(GetPassState() != RPI::PassState::Rendering, "LightCullingTilePreparePass: Trying to reload shader during rendering");
            if (GetPassState() == RPI::PassState::Idle)
            {
                ChooseShaderVariant();
            }
        }

        void LightCullingTilePreparePass::OnShaderReinitialized(const AZ::RPI::Shader&)
        {
            OnShaderReloaded();
        }

        void LightCullingTilePreparePass::OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>&)
        {
            OnShaderReloaded();
        }

        void LightCullingTilePreparePass::OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant&)
        {
            OnShaderReloaded();
        }

    }   // namespace Render
}   // namespace AZ
