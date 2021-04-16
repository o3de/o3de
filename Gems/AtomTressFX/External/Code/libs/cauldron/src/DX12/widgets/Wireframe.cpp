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
#include "Wireframe.h"
#include "../Base/ShaderCompilerHelper.h"

namespace CAULDRON_DX12
{
    Wireframe::Wireframe()
    {
    }

    Wireframe::~Wireframe()
    {
    }

    void Wireframe::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        DXGI_FORMAT outFormat,
        uint32_t sampleDescCount)
    {
        m_pResourceViewHeaps = pHeaps;
        m_pDynamicBufferRing = pDynamicBufferRing;


        D3D12_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // the vertex shader
        static const char* vertexShader = "\
        cbuffer cbPerObject: register(b0)\
        {\
            matrix        u_mWorldViewProj;\
            float4        u_Center;\
            float4        u_Radius;\
            float4        u_Color;\
        }\
        struct VERTEX_IN\
        {\
            float3 vPosition : POSITION;\
        };\
        struct VERTEX_OUT\
        {\
            float4 vColor : COLOR; \
            float4 vPosition : SV_POSITION; \
        }; \
        VERTEX_OUT mainVS(VERTEX_IN Input)\
        {\
            VERTEX_OUT Output;\
            Output.vPosition = mul(u_mWorldViewProj, float4(u_Center.xyz + Input.vPosition * u_Radius.xyz, 1.0f));\
            Output.vColor = u_Color;\
            return Output;\
        }";

        // the pixel shader
        static const char* pixelShader = "\
        struct VERTEX_IN\
        {\
            float4 vColor : COLOR; \
        }; \
        float4 mainPS(VERTEX_IN Input)  : SV_Target\
        {\
            return Input.vColor;\
        }";

        /////////////////////////////////////////////
        // Compile and create shaders

        D3D12_SHADER_BYTECODE shaderVert, shaderPixel;
        {
            CompileShaderFromString(vertexShader, NULL, "mainVS", "vs_5_0", 0, 0, &shaderVert);
            CompileShaderFromString(pixelShader, NULL, "mainPS", "ps_5_0", 0, 0, &shaderPixel);
        }

        /////////////////////////////////////////////
        // Create root signature
        {
            CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();

            CD3DX12_ROOT_PARAMETER RTSlot[1];
            RTSlot[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

            // the root signature contains 3 slots to be used        
            descRootSignature.NumParameters = 1;
            descRootSignature.pParameters = RTSlot;
            descRootSignature.NumStaticSamplers = 0;
            descRootSignature.pStaticSamplers = NULL;

            // deny uneccessary access to certain pipeline stages   
            descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
                | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

            ID3DBlob *pOutBlob, *pErrorBlob = NULL;
            D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);
            if (pErrorBlob != NULL)
            {
                char *msg = (char *)pErrorBlob->GetBufferPointer();
                MessageBoxA(0, msg, "", 0);
            }

            ThrowIfFailed(
                pDevice->GetDevice()->CreateRootSignature(
                    0, // node
                    pOutBlob->GetBufferPointer(),
                    pOutBlob->GetBufferSize(),
                    IID_PPV_ARGS(&m_RootSignature))
            );
            SetName(m_RootSignature, "Wireframe");

            pOutBlob->Release();
            if (pErrorBlob)
                pErrorBlob->Release();
        }

        /////////////////////////////////////////////
        // Create a PSO description

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
        descPso.InputLayout = { layout, 1 };
        descPso.pRootSignature = m_RootSignature;
        descPso.VS = shaderVert;
        descPso.PS = shaderPixel;
        descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descPso.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
        descPso.RasterizerState.AntialiasedLineEnable = TRUE;
        descPso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        descPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        descPso.SampleMask = UINT_MAX;
        descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        descPso.NumRenderTargets = 1;
        descPso.RTVFormats[0] = outFormat;
        descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        descPso.SampleDesc.Count = sampleDescCount;
        descPso.NodeMask = 0;
        ThrowIfFailed(
            pDevice->GetDevice()->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&m_pPipeline))
        );
        SetName(m_pPipeline, "Wireframe::m_pPipeline");
    }

    void Wireframe::OnDestroy()
    {
        m_pPipeline->Release();
        m_RootSignature->Release();
    }

    void Wireframe::Draw(ID3D12GraphicsCommandList* pCommandList, int numIndices, D3D12_INDEX_BUFFER_VIEW IBV, D3D12_VERTEX_BUFFER_VIEW VBV, XMMATRIX WorldViewProj, XMVECTOR vCenter, XMVECTOR vRadius, XMVECTOR vColor)
    {
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap() };

        pCommandList->IASetIndexBuffer(&IBV);
        pCommandList->IASetVertexBuffers(0, 1, &VBV);
        pCommandList->SetDescriptorHeaps(1, pDescriptorHeaps);
        pCommandList->SetPipelineState(m_pPipeline);
        pCommandList->SetGraphicsRootSignature(m_RootSignature);
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

        // Set per Object constants
        //
        per_object *cbPerObject;
        D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), (void **)&cbPerObject, &perObjectDesc);
        cbPerObject->m_mWorldViewProj = WorldViewProj;
        cbPerObject->m_vCenter = vCenter;
        cbPerObject->m_vRadius = vRadius;
        cbPerObject->m_vColor = vColor;

        pCommandList->SetGraphicsRootConstantBufferView(0, perObjectDesc);

        pCommandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
    }
}