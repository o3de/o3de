// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "../Base/Helper.h"
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/ShaderCompilerHelper.h"
#include "Misc/ThreadPool.h"

#include "PostProcPS.h"

namespace CAULDRON_DX12
{
    void PostProcPS::OnCreate(
        Device *pDevice,
        const std::string &shaderFilename,
        ResourceViewHeaps *pResourceViewHeaps,
        StaticBufferPool *pStaticBufferPool,
        uint32_t dwSRVTableSize,
        uint32_t dwStaticSamplersCount,
        D3D12_STATIC_SAMPLER_DESC *pStaticSamplers,
        DXGI_FORMAT outFormat,
        uint32_t psoSampleDescCount,
        D3D12_BLEND_DESC *pBlendDesc,
        D3D12_DEPTH_STENCIL_DESC *pDepthStencilDesc,
        uint32_t numRenderTargets
    )
    {
        m_pDevice = pDevice;
        m_pResourceViewHeaps = pResourceViewHeaps;

        float vertices[] = {
            -1,  1,  1,   0, 0,
             3,  1,  1,   2, 0,
            -1, -3,  1,   0, 2,
        };
        pStaticBufferPool->AllocVertexBuffer(3, 5 * sizeof(float), vertices, &m_verticesView);

        // Create the vertex shader
        static const char* vertexShader =
            "struct VERTEX_IN\
                {\
                    float3 vPosition : POSITION;\
                    float2 vTexture  : TEXCOORD;\
                };\
                struct VERTEX_OUT\
                {\
                    float2 vTexture : TEXCOORD;\
                    float4 vPosition : SV_POSITION;\
                };\
                VERTEX_OUT mainVS(VERTEX_IN Input)\
                {\
                    VERTEX_OUT Output;\
                    Output.vPosition = float4(Input.vPosition, 1.0f);\
                    Output.vTexture = Input.vTexture;\
                    return Output;\
                }";

        // Compile shaders
        //
        {
            DefineList defines;
            CompileShaderFromString(vertexShader, &defines, "mainVS", "vs_5_0", 0, 0, &m_shaderVert);
            CompileShaderFromFile(shaderFilename.c_str(), &defines, "mainPS", "ps_5_0", 0, &m_shaderPixel);
        }

        // Create root signature
        //
        {
            int numParams = 0;

            CD3DX12_DESCRIPTOR_RANGE DescRange[3];
            DescRange[numParams++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);                   // b0 <- per frame
            if (dwSRVTableSize > 0)
            {
                DescRange[numParams++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, dwSRVTableSize, 0);  // t0 <- per material
            }

            CD3DX12_ROOT_PARAMETER RTSlot[3];
            RTSlot[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
            for (int i = 1; i < numParams; i++)
                RTSlot[i].InitAsDescriptorTable(1, &DescRange[i], D3D12_SHADER_VISIBILITY_ALL);

            // the root signature contains 3 slots to be used
            CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
            descRootSignature.NumParameters = numParams;
            descRootSignature.pParameters = RTSlot;
            descRootSignature.NumStaticSamplers = dwStaticSamplersCount;
            descRootSignature.pStaticSamplers = pStaticSamplers;

            // deny uneccessary access to certain pipeline stages   
            descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ID3DBlob *pOutBlob, *pErrorBlob = NULL;
            ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
            ThrowIfFailed(
                pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))
            );
            SetName(m_pRootSignature, std::string("PostProcPS::") + shaderFilename);

            pOutBlob->Release();
            if (pErrorBlob)
                pErrorBlob->Release();
        }

        UpdatePipeline(outFormat, pBlendDesc, pDepthStencilDesc, psoSampleDescCount, numRenderTargets);
    }

    void PostProcPS::UpdatePipeline(DXGI_FORMAT outFormat, D3D12_BLEND_DESC *pBlendDesc, D3D12_DEPTH_STENCIL_DESC *pDepthStencilDesc, uint32_t psoSampleDescCount, uint32_t numRenderTargets)
    {
        if (outFormat == DXGI_FORMAT_UNKNOWN)
            return;

        if (m_pPipeline != NULL)
        {
            m_pPipeline->Release();
            m_pPipeline = NULL;
        }

        //we need to cache the refrenced data so the lambda function can get a copy
        D3D12_BLEND_DESC blendDesc = pBlendDesc ? *pBlendDesc : CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC depthStencilBlankDesc = {};
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = pDepthStencilDesc ? *pDepthStencilDesc : depthStencilBlankDesc;

        // layout
        D3D12_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
        descPso.InputLayout = { layout, 2 };
        descPso.pRootSignature = m_pRootSignature;
        descPso.VS = m_shaderVert;
        descPso.PS = m_shaderPixel;
        descPso.DepthStencilState = depthStencilDesc;
        if (depthStencilDesc.DepthEnable == TRUE)
        {
            descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        }
        descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descPso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        descPso.BlendState = blendDesc;
        descPso.SampleMask = UINT_MAX;
        descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descPso.NumRenderTargets = numRenderTargets;
        for (uint32_t i = 0; i < numRenderTargets; ++i)
            descPso.RTVFormats[i] = outFormat;
        descPso.SampleDesc.Count = psoSampleDescCount;
        descPso.NodeMask = 0;
        ThrowIfFailed(
            m_pDevice->GetDevice()->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&m_pPipeline))
        );
        SetName(m_pPipeline, "PostProcPS::m_pPipeline");
    }

    void PostProcPS::OnDestroy()
    {
        if (m_pPipeline != NULL)
        {
            m_pPipeline->Release();
            m_pPipeline = NULL;
        }

        if (m_pRootSignature != NULL)
        {
            m_pRootSignature->Release();
            m_pRootSignature = NULL;
        }
    }

    void PostProcPS::Draw(ID3D12GraphicsCommandList* pCommandList, uint32_t dwSRVTableSize, CBV_SRV_UAV *pSRVTable, D3D12_GPU_VIRTUAL_ADDRESS constantBuffer)
    {
        if (m_pPipeline == NULL)
            return;

        // Bind vertices 
        //
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pCommandList->IASetVertexBuffers(0, 1, &m_verticesView);

        // Bind Descriptor heaps, root signatures and descriptor sets
        //                
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap(), m_pResourceViewHeaps->GetSamplerHeap() };
        pCommandList->SetDescriptorHeaps(2, pDescriptorHeaps);
        pCommandList->SetGraphicsRootSignature(m_pRootSignature);
        pCommandList->SetGraphicsRootConstantBufferView(0, constantBuffer);
        if (dwSRVTableSize > 0)
            pCommandList->SetGraphicsRootDescriptorTable(1, pSRVTable->GetGPU());

        // Bind Pipeline
        //
        pCommandList->SetPipelineState(m_pPipeline);

        // Draw
        //
        pCommandList->DrawInstanced(3, 1, 0, 0);
    }
}