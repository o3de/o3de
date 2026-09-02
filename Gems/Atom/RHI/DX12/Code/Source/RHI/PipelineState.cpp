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

#if defined(O3DE_DX12_MESH_SHADER_SUPPORT)
        namespace
        {
            // One void*-aligned { type-tag, payload } entry of a D3D12 pipeline-state stream.
            // Hand-rolled because the vendored d3dx12.h ships no CD3DX12_PIPELINE_STATE_STREAM
            // helpers (validated by the Phase-0 mesh-shader spike).
            template<typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
            struct alignas(void*) StreamSubobject
            {
                D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_type = Type;
                T m_value{};
            };

            // Subobject set for a mesh-shader graphics PSO: amplification (optional) + mesh +
            // fragment, plus the standard render state. Deliberately has NO input-layout and NO
            // primitive-topology-type subobject (D3D12 rejects those on the mesh path).
            struct MeshPipelineStream
            {
                StreamSubobject<ID3D12RootSignature*,  D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>       m_rootSignature;
                StreamSubobject<UINT,                  D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK>            m_nodeMask;
                StreamSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>                   m_as;
                StreamSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>                   m_ms;
                StreamSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>                   m_ps;
                StreamSubobject<D3D12_BLEND_DESC,      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>                m_blend;
                StreamSubobject<UINT,                  D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK>          m_sampleMask;
                StreamSubobject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>           m_rasterizer;
                StreamSubobject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL>     m_depthStencil;
                StreamSubobject<DXGI_FORMAT,           D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> m_dsvFormat;
                StreamSubobject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> m_rtFormats;
                StreamSubobject<DXGI_SAMPLE_DESC,      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC>          m_sampleDesc;
            };
        } // anonymous namespace
#endif // O3DE_DX12_MESH_SHADER_SUPPORT

        RHI::ResultCode PipelineState::InitInternal(
            RHI::Device& deviceBase,
            const RHI::PipelineStateDescriptorForDraw& descriptor,
            RHI::DevicePipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);

#if defined(O3DE_DX12_MESH_SHADER_SUPPORT)
            // Mesh-shader PSO: a graphics PSO driven by mesh (+ optional amplification) instead of
            // vertex/geometry input-assembly. Built via the stream-subobject path (the legacy
            // D3D12_GRAPHICS_PIPELINE_STATE_DESC has no MS/AS slots). The PipelineLibrary/PSO cache
            // is skipped (it only knows the legacy desc; PSO caching is already disabled for DX12).
            if (descriptor.m_meshFunction)
            {
                RHI::ConstPtr<PipelineLayout> meshPipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor, /*forceMeshRootSignatureFlags*/ true);
                AZStd::vector<ShaderByteCode> meshShaderByteCodeCache;

                MeshPipelineStream stream;
                stream.m_rootSignature.m_value = meshPipelineLayout->Get();
                stream.m_nodeMask.m_value = 1;

                if (const ShaderStageFunction* amplificationFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_amplificationFunction.get()))
                {
                    stream.m_as.m_value = D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*amplificationFunction, descriptor, meshShaderByteCodeCache));
                }
                if (const ShaderStageFunction* meshFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_meshFunction.get()))
                {
                    stream.m_ms.m_value = D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*meshFunction, descriptor, meshShaderByteCodeCache));
                }
                if (const ShaderStageFunction* fragmentFunction = azrtti_cast<const ShaderStageFunction*>(descriptor.m_fragmentFunction.get()))
                {
                    stream.m_ps.m_value = D3D12BytecodeFromView(ShaderUtils::PatchShaderFunction(*fragmentFunction, descriptor, meshShaderByteCodeCache));
                }

                stream.m_blend.m_value = ConvertBlendState(descriptor.m_renderStates.m_blendState);
                stream.m_sampleMask.m_value = 0xFFFFFFFFu;
                stream.m_rasterizer.m_value = ConvertRasterState(descriptor.m_renderStates.m_rasterState);
                stream.m_depthStencil.m_value = ConvertDepthStencilState(descriptor.m_renderStates.m_depthStencilState);

                const RHI::RenderAttachmentConfiguration& meshRenderAttachmentConfiguration = descriptor.m_renderAttachmentConfiguration;
                stream.m_dsvFormat.m_value = ConvertFormat(meshRenderAttachmentConfiguration.GetDepthStencilFormat());

                D3D12_RT_FORMAT_ARRAY rtFormats = {};
                rtFormats.NumRenderTargets = meshRenderAttachmentConfiguration.GetRenderTargetCount();
                for (uint32_t targetIdx = 0; targetIdx < rtFormats.NumRenderTargets; ++targetIdx)
                {
                    rtFormats.RTFormats[targetIdx] = ConvertFormat(meshRenderAttachmentConfiguration.GetRenderTargetFormat(targetIdx));
                }
                stream.m_rtFormats.m_value = rtFormats;

                stream.m_sampleDesc.m_value.Count = descriptor.m_renderStates.m_multisampleState.m_samples;
                stream.m_sampleDesc.m_value.Quality = descriptor.m_renderStates.m_multisampleState.m_quality;

                D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
                streamDesc.SizeInBytes = sizeof(stream);
                streamDesc.pPipelineStateSubobjectStream = &stream;

                Microsoft::WRL::ComPtr<ID3D12PipelineState> meshPipelineStateComPtr;
                const HRESULT meshPsoHr = device.GetDevice()->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(meshPipelineStateComPtr.GetAddressOf()));
                RHI::Ptr<ID3D12PipelineState> meshPipelineState = meshPipelineStateComPtr.Get();

                if (meshPipelineState)
                {
                    m_pipelineLayout = AZStd::move(meshPipelineLayout);
                    m_pipelineState = AZStd::move(meshPipelineState);
                    m_pipelineStateData.m_type = RHI::PipelineStateType::Draw;
                    // The mesh path has no input assembler; carry a valid (but unused) topology so the
                    // draw-time IASetPrimitiveTopology call stays legal.
                    m_pipelineStateData.m_drawData = PipelineStateDrawData{ descriptor.m_renderStates.m_multisampleState, RHI::PrimitiveTopology::TriangleList };
                    return RHI::ResultCode::Success;
                }

                // Diagnostic (no debug layer needed): dump the HRESULT + the key stream inputs so the
                // failure cause is unambiguous in the normal log. hr=0x80070057 (E_INVALIDARG) with a
                // complete desc (msLen>0, psLen>0, rtCount matches the PS outputs, rootSig!=null) almost
                // always means the root signature kept ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT -- i.e. the
                // Mesh/Amplification SRG visibility was not reflected, so PipelineLayout did not drop the
                // IA flag (which D3D12 forbids on a mesh PSO). See PipelineLayout.cpp usesMeshStage log.
                AZ_Error("PipelineState", false,
                    "Failed to compile mesh-shader pipeline state: CreatePipelineState hr=0x%08X | "
                    "asLen=%zu msLen=%zu psLen=%zu | rtCount=%u dsvFmt=%d samples=%u | rootSig=%p",
                    static_cast<unsigned int>(meshPsoHr),
                    static_cast<size_t>(stream.m_as.m_value.BytecodeLength),
                    static_cast<size_t>(stream.m_ms.m_value.BytecodeLength),
                    static_cast<size_t>(stream.m_ps.m_value.BytecodeLength),
                    static_cast<unsigned int>(rtFormats.NumRenderTargets),
                    static_cast<int>(stream.m_dsvFormat.m_value),
                    static_cast<unsigned int>(stream.m_sampleDesc.m_value.Count),
                    static_cast<void*>(stream.m_rootSignature.m_value));
                return RHI::ResultCode::Fail;
            }
#endif // O3DE_DX12_MESH_SHADER_SUPPORT

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
