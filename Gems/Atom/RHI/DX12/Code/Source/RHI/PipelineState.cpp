/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/PipelineState.h>
#include <RHI/PipelineLibrary.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ShaderUtils.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<PipelineState> PipelineState::Create()
        {
            return aznew PipelineState;
        }

        const PipelineLayout* PipelineState::GetPipelineLayout() const
        {
            return m_pipelineLayout.get();
        }

        ID3D12PipelineState* PipelineState::Get() const
        {
            return m_pipelineState.get();
        }

        const PipelineStateData& PipelineState::GetPipelineStateData() const
        {
            return m_pipelineStateData;
        }

        D3D12_SHADER_BYTECODE D3D12BytecodeFromView(ShaderByteCodeView view)
        {
            return D3D12_SHADER_BYTECODE{ view.data(), view.size() };
        }

        RHI::ResultCode PipelineState::InitInternal(
            RHI::Device& deviceBase,
            const RHI::PipelineStateDescriptorForDraw& descriptor,
            RHI::DevicePipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
            pipelineStateDesc.NodeMask = 1;
            pipelineStateDesc.SampleMask = 0xFFFFFFFFu;
            pipelineStateDesc.SampleDesc.Count = descriptor.m_renderStates.m_multisampleState.m_samples;
            pipelineStateDesc.SampleDesc.Quality = descriptor.m_renderStates.m_multisampleState.m_quality;

            // Shader state.
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            pipelineStateDesc.pRootSignature = pipelineLayout->Get();
            // Cache used for saving the patched version of the shader
            AZStd::vector<ShaderByteCode> shaderByteCodeCache;
            if (const ShaderStageFunction* vertexFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_vertexFunction.get()))
            {
                pipelineStateDesc.VS =
                    D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*vertexFunction, descriptor, shaderByteCodeCache));
            }

            if (const ShaderStageFunction* geometryFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_geometryFunction.get()))
            {
                pipelineStateDesc.GS =
                    D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*geometryFunction, descriptor, shaderByteCodeCache));
            }

            if (const ShaderStageFunction* fragmentFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_fragmentFunction.get()))
            {
                pipelineStateDesc.PS =
                    D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*fragmentFunction, descriptor, shaderByteCodeCache));
            }

            const RHI::RenderAttachmentConfiguration& renderAttachmentConfiguration = descriptor.m_renderAttachmentConfiguration;

            pipelineStateDesc.DSVFormat = ConvertFormat(renderAttachmentConfiguration.GetDepthStencilFormat());
            pipelineStateDesc.NumRenderTargets = renderAttachmentConfiguration.GetRenderTargetCount();
            for (uint32_t targetIdx = 0; targetIdx < pipelineStateDesc.NumRenderTargets; ++targetIdx)
            {
                pipelineStateDesc.RTVFormats[targetIdx] = ConvertFormat(renderAttachmentConfiguration.GetRenderTargetFormat(targetIdx));
            }

            AZStd::vector<D3D12_INPUT_ELEMENT_DESC> inputElements = ConvertInputElements(descriptor.m_inputStreamLayout);
            pipelineStateDesc.InputLayout.NumElements = uint32_t(inputElements.size());
            pipelineStateDesc.InputLayout.pInputElementDescs = inputElements.data();
            pipelineStateDesc.PrimitiveTopologyType = ConvertToTopologyType(descriptor.m_inputStreamLayout.GetTopology());

            pipelineStateDesc.BlendState = ConvertBlendState(descriptor.m_renderStates.m_blendState);
            pipelineStateDesc.RasterizerState = ConvertRasterState(descriptor.m_renderStates.m_rasterState);
            pipelineStateDesc.DepthStencilState = ConvertDepthStencilState(descriptor.m_renderStates.m_depthStencilState);

            PipelineLibrary* pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibraryBase);

            RHI::Ptr<ID3D12PipelineState> pipelineState;
            if (pipelineLibrary && pipelineLibrary->IsInitialized())
            {
                pipelineState = pipelineLibrary->CreateGraphicsPipelineState(static_cast<uint64_t>(descriptor.GetHash()), pipelineStateDesc);
            }
            else
            {
                Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateComPtr;
                device.AssertSuccess(device.GetDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf())));
                pipelineState = pipelineStateComPtr.Get();
            }

            if (pipelineState)
            {
                m_pipelineLayout = AZStd::move(pipelineLayout);
                m_pipelineState = AZStd::move(pipelineState);
                m_pipelineStateData.m_type = RHI::PipelineStateType::Draw;
                m_pipelineStateData.m_drawData = PipelineStateDrawData{ descriptor.m_renderStates.m_multisampleState, descriptor.m_inputStreamLayout.GetTopology() };
                return RHI::ResultCode::Success;
            }
            else
            {
                AZ_Error("PipelineState", false, "Failed to compile graphics pipeline state. Check the D3D12 debug layer for more info.");
                return RHI::ResultCode::Fail;
            }
        }

        RHI::ResultCode PipelineState::InitInternal(
            RHI::Device& deviceBase,
            const RHI::PipelineStateDescriptorForDispatch& descriptor,
            RHI::DevicePipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);

            D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
            pipelineStateDesc.NodeMask = 1;

            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            pipelineStateDesc.pRootSignature = pipelineLayout->Get();
            // Cache used for saving the patched version of the shader
            AZStd::vector<ShaderByteCode> shaderByteCodeCache;
            if (const ShaderStageFunction* computeFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_computeFunction.get()))
            {
                pipelineStateDesc.CS =
                    D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*computeFunction, descriptor, shaderByteCodeCache));
            }

            PipelineLibrary* pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibraryBase);

            RHI::Ptr<ID3D12PipelineState> pipelineState;
            if (pipelineLibrary && pipelineLibrary->IsInitialized())
            {
                pipelineState = pipelineLibrary->CreateComputePipelineState(static_cast<uint64_t>(descriptor.GetHash()), pipelineStateDesc);
            }
            else
            {
                Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateComPtr;
                device.AssertSuccess(device.GetDevice()->CreateComputePipelineState(&pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf())));
                pipelineState = pipelineStateComPtr.Get();
            }

            if (pipelineState)
            {
                m_pipelineLayout = AZStd::move(pipelineLayout);
                m_pipelineState = AZStd::move(pipelineState);
                m_pipelineStateData.m_type = RHI::PipelineStateType::Dispatch;
                return RHI::ResultCode::Success;
            }
            else
            {
                AZ_Error("PipelineState", false, "Failed to compile graphics pipeline state. Check the D3D12 debug layer for more info.");
                return RHI::ResultCode::Fail;
            }
        }

        RHI::ResultCode PipelineState::InitInternal(
            RHI::Device& deviceBase,
            const RHI::PipelineStateDescriptorForRayTracing& descriptor,
            [[maybe_unused]] RHI::DevicePipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);

            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);

            m_pipelineLayout = AZStd::move(pipelineLayout);
            m_pipelineStateData.m_type = RHI::PipelineStateType::RayTracing;

            return RHI::ResultCode::Success;
        }

        void PipelineState::ShutdownInternal()
        {
            // ray tracing shaders do not have a traditional pipeline state object
            if (m_pipelineStateData.m_type != RHI::PipelineStateType::RayTracing)
            {
                static_cast<Device&>(GetDevice()).QueueForRelease(AZStd::move(m_pipelineState));
            }

            m_pipelineState = nullptr;
            m_pipelineLayout = nullptr;
        }
    }
}
