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
#include "Base/Device.h"
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/UserMarkers.h"
#include "Base/UploadHeap.h"
#include "Base/Texture.h"
#include "Base/Helper.h"
#include "Base/Imgui.h"
#include "PostProcPS.h"
#include "Bloom.h"

namespace CAULDRON_DX12
{
    void Bloom::OnCreate(
        Device                 *pDevice,
        ResourceViewHeaps      *pHeaps,
        DynamicBufferRing      *pConstantBufferRing,
        StaticBufferPool       *pStaticBufferPool,        
        DXGI_FORMAT             outFormat
    )
    {
        m_pDevice = pDevice;
        m_pResourceViewHeaps = pHeaps;
        m_pConstantBufferRing = pConstantBufferRing;
        m_outFormat = outFormat;

        m_blur.OnCreate(pDevice, m_pResourceViewHeaps, pConstantBufferRing, pStaticBufferPool, m_outFormat);

        //blending add
        {
            D3D12_BLEND_DESC blendingFactor = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

            blendingFactor.IndependentBlendEnable = TRUE;
            blendingFactor.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC
            {
                TRUE,FALSE,
                D3D12_BLEND_ONE, D3D12_BLEND_BLEND_FACTOR, D3D12_BLEND_OP_ADD,
                D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                D3D12_COLOR_WRITE_ENABLE_ALL,
            };

            D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};
            SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            SamplerDesc.MinLOD = 0.0f;
            SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            SamplerDesc.MipLODBias = 0;
            SamplerDesc.MaxAnisotropy = 1;
            SamplerDesc.ShaderRegister = 0;
            SamplerDesc.RegisterSpace = 0;
            SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            m_blendFactor.OnCreate(m_pDevice, "blend.hlsl", m_pResourceViewHeaps, pStaticBufferPool, 1, 1, &SamplerDesc, m_outFormat, 1, &blendingFactor);
        }

        // Allocate descriptors for the mip chain
        //
        for (int i = 0; i < BLOOM_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_mip[i].m_SRV);
            m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_mip[i].m_RTV);
        }

        // Allocate descriptors for the output pass
        //
        m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_output.m_SRV);
        m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_output.m_RTV);

        m_doBlur = true;
        m_doUpscale = true;
    }

    void Bloom::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture *pInput, int mipCount, Texture *pOutput)
    {
        m_Width = Width;
        m_Height = Height;
        m_mipCount = mipCount;
        m_pInput = pInput;
        m_pOutput = pOutput;

        m_blur.OnCreateWindowSizeDependentResources(m_pDevice, Width, Height, pInput, mipCount);

        // Create views for the mip chain
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            pInput->CreateSRV(0, &m_mip[i].m_SRV, i);
            pInput->CreateRTV(0, &m_mip[i].m_RTV, i);
        }

        // Create views for the output pass
        //
        {
            pInput->CreateSRV(0, &m_output.m_SRV, 0);
            pOutput->CreateRTV(0, &m_output.m_RTV, 0);
        }

        // set weights of each mip level
        m_mip[0].m_weight = 1.0f - 0.08f;
        m_mip[1].m_weight = 0.25f;
        m_mip[2].m_weight = 0.75f;
        m_mip[3].m_weight = 1.5f;
        m_mip[4].m_weight = 2.5f;
        m_mip[5].m_weight = 3.0f;

        // normalize weights
        float n = 0;
        for (uint32_t i = 1; i < 6; i++)
            n += m_mip[i].m_weight;

        for (uint32_t i = 1; i < 6; i++)
            m_mip[i].m_weight /= n;
    }

    void Bloom::OnDestroyWindowSizeDependentResources()
    {
        m_blur.OnDestroyWindowSizeDependentResources();
    }

    void Bloom::OnDestroy()
    {
        m_blur.OnDestroy();
        m_blendFactor.OnDestroy();
    }

    void Bloom::Draw(ID3D12GraphicsCommandList* pCommandList, Texture *pInput)
    {
        UserMarker marker(pCommandList, "Bloom");

        //float weights[6] = { 0.25, 0.75, 1.5, 2, 2.5, 3.0 };

        // given a RT, and its mip chain m0, m1, m2, m3, m4 
        // 
        // m4 = blur(m5)
        // m4 = blur(m4) + w5 *m5
        // m3 = blur(m3) + w4 *m4
        // m2 = blur(m2) + w3 *m3
        // m1 = blur(m1) + w2 *m2
        // m0 = blur(m0) + w1 *m1
        // RT = 0.92 * RT + 0.08 * m0

        // blend and upscale
        for (int i = m_mipCount - 1; i >= 0; i--)
        {
            // blur mip level
            //
            if (m_doBlur)
            {
                m_blur.Draw(pCommandList, i);
            }

            // blend with mip above   
            //SetPerfMarkerBegin(cmd_buf, "blend above");

            Bloom::cbBlend *data;
            D3D12_GPU_VIRTUAL_ADDRESS constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(Bloom::cbBlend), (void **)&data, &constantBuffer);

            if (i != 0)
            {
                pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pInput->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, i - 1));

                pCommandList->OMSetRenderTargets(1, &m_mip[i - 1].m_RTV.GetCPU(), true, NULL);

                SetViewportAndScissor(pCommandList, 0, 0, m_Width >> (i - 1), m_Height >> (i - 1));

                float blendConstants[4] = { m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight };
                pCommandList->OMSetBlendFactor(blendConstants);

                data->weight = 1.0f;

                if (m_doUpscale)
                    m_blendFactor.Draw(pCommandList, 1, &m_mip[i].m_SRV, constantBuffer);


                pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pInput->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, i - 1));

            }
            else
            {
                pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pOutput->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

                //composite
                pCommandList->OMSetRenderTargets(1, &m_output.m_RTV.GetCPU(), true, NULL);

                SetViewportAndScissor(pCommandList, 0, 0, m_Width * 2, m_Height * 2);

                float blendConstants[4] = { m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight };
                pCommandList->OMSetBlendFactor(blendConstants);

                data->weight = 1.0f - m_mip[i].m_weight;

                if (m_doUpscale)
                    m_blendFactor.Draw(pCommandList, 1, &m_mip[i].m_SRV, constantBuffer);

                pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pOutput->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
            }
        }
    }

    void Bloom::Gui()
    {
        bool opened = true;
        if (ImGui::Begin("Bloom Controls", &opened))
        {
            ImGui::Checkbox("Blur Bloom Stages", &m_doBlur);
            ImGui::Checkbox("Upscaling", &m_doUpscale);

            ImGui::SliderFloat("weight 0", &m_mip[0].m_weight, 0.0f, 1.0f);

            for (int i = 1; i < m_mipCount; i++)
            {
                char buf[32];
                sprintf_s<32>(buf, "weight %i", i);
                ImGui::SliderFloat(buf, &m_mip[i].m_weight, 0.0f, 4.0f);
            }
        }
        ImGui::End();
    }
}
