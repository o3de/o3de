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
#include "Imgui.h"

// DirectX
#include <d3d11.h>
#include "../Base/ShaderCompilerHelper.h"

namespace CAULDRON_DX12
{
    struct VERTEX_CONSTANT_BUFFER
    {
        float        mvp[4][4];
    };

    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::OnCreate(Device *pDevice, UploadHeap *pUploadHeap, ResourceViewHeaps *pHeaps, DynamicBufferRing *pConstantBufferRing, DXGI_FORMAT outFormat)
    {
        m_pResourceViewHeaps = pHeaps;
        m_pConstBuf = pConstantBufferRing;
        m_pDevice = pDevice;

        // Get UI texture 
        //
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Create the texture object
        //
        CD3DX12_RESOURCE_DESC RDescs = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
        {
            m_pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &RDescs,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_pTexture2D)
            );
        }

        // Create the Image View
        //
        {
            m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pTextureSRV);
            pDevice->GetDevice()->CreateShaderResourceView(m_pTexture2D, nullptr, m_pTextureSRV.GetCPU());
        }

        // Tell ImGUI what the image view is
        //
        io.Fonts->TexID = (ImTextureID)&m_pTextureSRV;

        // Allocate memory int upload heap and copy the texture into it
        //
        UINT64 UplHeapSize;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex2D[1] = { 0 };
        uint32_t num_rows[1] = { 0 };
        UINT64 row_sizes_in_bytes[1] = { 0 };
        m_pDevice->GetDevice()->GetCopyableFootprints(&RDescs, 0, 1, 0, placedTex2D, num_rows, row_sizes_in_bytes, &UplHeapSize);

        UINT8* ptr = pUploadHeap->Suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        placedTex2D[0].Offset += UINT64(ptr - pUploadHeap->BasePtr());

        memcpy(ptr, pixels, width*height * 4);

        // Copy from upload heap into the vid mem image
        //
        {
            {
                // prepare to copy dest
                D3D12_RESOURCE_BARRIER RBDesc = {};
                RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                RBDesc.Transition.pResource = m_pTexture2D;
                RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                pUploadHeap->GetCommandList()->ResourceBarrier(1, &RBDesc);
            }

            // copy upload texture to texture heap
            for (uint32_t mip = 0; mip < 1; ++mip)
            {
                D3D12_RESOURCE_DESC texDesc = m_pTexture2D->GetDesc();
                CD3DX12_TEXTURE_COPY_LOCATION Dst(m_pTexture2D, mip);
                CD3DX12_TEXTURE_COPY_LOCATION Src(pUploadHeap->GetResource(), placedTex2D[mip]);
                pUploadHeap->GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, NULL);
            }

            {
                // prepare to shader read
                D3D12_RESOURCE_BARRIER RBDesc = {};
                RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                RBDesc.Transition.pResource = m_pTexture2D;
                RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                pUploadHeap->GetCommandList()->ResourceBarrier(1, &RBDesc);
            }
        }

        // Kick off the upload
        //
        pUploadHeap->FlushAndFinish();

        // Create sampler
        //
        D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};
        SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        SamplerDesc.MipLODBias = 0;
        SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        SamplerDesc.MinLOD = 0.0f;
        SamplerDesc.MaxLOD = 0.0f;
        SamplerDesc.ShaderRegister = 0;
        SamplerDesc.RegisterSpace = 0;
        SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Vertex shader
        //
        static const char* vertexShader =
            "cbuffer vertexBuffer : register(b0) \
        {\
        float4x4 ProjectionMatrix; \
        };\
        struct VS_INPUT\
        {\
        float2 pos : POSITION;\
        float2 uv  : TEXCOORD;\
        float4 col : COLOR;\
        };\
        \
        struct PS_INPUT\
        {\
        float4 pos : SV_POSITION;\
        float2 uv  : TEXCOORD;\
        float4 col : COLOR;\
        };\
        \
        PS_INPUT main(VS_INPUT input)\
        {\
        PS_INPUT output;\
        output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
        output.col = input.col;\
        output.uv  = input.uv;\
        return output;\
        }";

        // Pixel shader
        //
        static const char* pixelShader =
            "struct PS_INPUT\
        {\
        float4 pos : SV_POSITION;\
        float2 uv  : TEXCOORD;\
        float4 col : COLOR;\
        };\
        sampler sampler0;\
        Texture2D texture0;\
        \
        float4 main(PS_INPUT input) : SV_Target\
        {\
        float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
        return out_col; \
        }";

        // Compile and create shaders
        //        
        CompileShaderFromString(vertexShader, NULL, "main", "vs_5_0", 0, 0, &m_shaderVert);
        CompileShaderFromString(pixelShader, NULL, "main", "ps_5_0", 0, 0, &m_shaderPixel);

        // Create descriptor sets
        //
        {
            // create a root signature with buffer slots for constants and sampler and Texture
            Microsoft::WRL::ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

            CD3DX12_DESCRIPTOR_RANGE DescRange[2];
            DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);        // b0 <- per frame
            DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);        // t0 <- per material

            // Visibility to all stages allows sharing binding tables
            CD3DX12_ROOT_PARAMETER RTSlot[2];
            RTSlot[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
            RTSlot[1].InitAsDescriptorTable(1, &DescRange[1], D3D12_SHADER_VISIBILITY_ALL);

            // the root signature contains 3 slots to be used
            CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
            descRootSignature.NumParameters = 2;
            descRootSignature.pParameters = RTSlot;
            descRootSignature.NumStaticSamplers = 1;
            descRootSignature.pStaticSamplers = &SamplerDesc;

            // deny uneccessary access to certain pipeline stages
            descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
                | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

            HRESULT hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);
            m_pDevice->GetDevice()->CreateRootSignature(
                0,
                pOutBlob->GetBufferPointer(),
                pOutBlob->GetBufferSize(),
                __uuidof(ID3D12RootSignature),
                (void**)&m_pRootSignature);
            SetName(m_pRootSignature, "ImGUI::m_RootSignature");
        }

        UpdatePipeline(outFormat);
    }

    //--------------------------------------------------------------------------------------
    //
    // UpdatePipeline
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::UpdatePipeline(DXGI_FORMAT outFormat)
    {
        if (outFormat == DXGI_FORMAT_UNKNOWN)
            return;

        if (m_pPipelineState != NULL)
        {
            m_pPipelineState->Release();
            m_pPipelineState = NULL;
        }

        // Create the input layout
        D3D12_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        uint32_t numElements = sizeof(layout) / sizeof(layout[0]);

        // Create the pipeline
        //
        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
        descPso.InputLayout = { layout, numElements };
        descPso.pRootSignature = m_pRootSignature;
        descPso.VS = m_shaderVert;
        descPso.PS = m_shaderPixel;
        descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descPso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        descPso.RasterizerState.DepthClipEnable = true;
        descPso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        descPso.BlendState.RenderTarget[0].BlendEnable = true;
        descPso.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        descPso.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        descPso.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        descPso.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        descPso.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        descPso.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        descPso.DepthStencilState.DepthEnable = FALSE;
        descPso.SampleMask = UINT_MAX;
        descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descPso.NumRenderTargets = 1;
        descPso.RTVFormats[0] = outFormat;
        descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        descPso.SampleDesc.Count = 1;
        descPso.NodeMask = 0;
        m_pDevice->GetDevice()->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&m_pPipelineState));
        SetName(m_pPipelineState, "ImGUI::m_pPipelineState");
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::OnDestroy()
    {
        if (!m_pDevice)
            return;

        m_pDevice = NULL;
        if (m_pPipelineState)
        {
            m_pPipelineState->Release();
            m_pPipelineState = NULL;
        }

        if (m_pRootSignature)
        {
            m_pRootSignature->Release();
            m_pRootSignature = NULL;
        }
        if (m_pTexture2D)
        {
            m_pTexture2D->Release();
            m_pTexture2D = NULL;
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::Draw(ID3D12GraphicsCommandList *pCommandList)
    {
        UserMarker marker(pCommandList, "ImGUI");

        ImGui::Render();

        ImDrawData* draw_data = ImGui::GetDrawData();

        // Create and grow vertex/index buffers if needed
        char *pVertices = NULL;
        D3D12_VERTEX_BUFFER_VIEW VerticesView;
        m_pConstBuf->AllocVertexBuffer(draw_data->TotalVtxCount, sizeof(ImDrawVert), (void **)&pVertices, &VerticesView);

        char *pIndices = NULL;
        D3D12_INDEX_BUFFER_VIEW IndicesView;
        m_pConstBuf->AllocIndexBuffer(draw_data->TotalIdxCount, sizeof(ImDrawIdx), (void **)&pIndices, &IndicesView);

        ImDrawVert* vtx_dst = (ImDrawVert*)pVertices;
        ImDrawIdx* idx_dst = (ImDrawIdx*)pIndices;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }

        // Setup orthographic projection matrix into our constant buffer
        D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferGpuDescriptor;
        {
            VERTEX_CONSTANT_BUFFER* constant_buffer;
            m_pConstBuf->AllocConstantBuffer(sizeof(VERTEX_CONSTANT_BUFFER), (void **)&constant_buffer, &ConstantBufferGpuDescriptor);

            float L = 0.0f;
            float R = ImGui::GetIO().DisplaySize.x;
            float B = ImGui::GetIO().DisplaySize.y;
            float T = 0.0f;
            float mvp[4][4] =
            {
                { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
            };
            memcpy(constant_buffer->mvp, mvp, sizeof(mvp));
        }

        // Setup viewport
        D3D12_VIEWPORT vp;
        memset(&vp, 0, sizeof(D3D12_VIEWPORT));
        vp.Width = ImGui::GetIO().DisplaySize.x;
        vp.Height = ImGui::GetIO().DisplaySize.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        pCommandList->RSSetViewports(1, &vp);

        // set pipeline & render state
        pCommandList->SetPipelineState(m_pPipelineState);
        pCommandList->SetGraphicsRootSignature(m_pRootSignature);

        pCommandList->IASetIndexBuffer(&IndicesView);
        pCommandList->IASetVertexBuffers(0, 1, &VerticesView);
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D12DescriptorHeap* pDH[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap(), m_pResourceViewHeaps->GetSamplerHeap() };
        pCommandList->SetDescriptorHeaps(2, &pDH[0]);
        pCommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferGpuDescriptor); // set CB	

        // Render command lists
        int vtx_offset = 0;
        int idx_offset = 0;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    const D3D12_RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };
                    pCommandList->RSSetScissorRects(1, &r);
                    pCommandList->SetGraphicsRootDescriptorTable(1, ((CBV_SRV_UAV*)(pcmd->TextureId))->GetGPU());

                    pCommandList->DrawIndexedInstanced(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
                }
                idx_offset += pcmd->ElemCount;
            }
            vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }
}
