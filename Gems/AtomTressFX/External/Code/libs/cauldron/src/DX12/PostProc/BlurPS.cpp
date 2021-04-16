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
#include "../Base/DynamicBufferRing.h"
#include "../Base/StaticBufferPool.h"
#include "../Base/UploadHeap.h"
#include "../Base/Texture.h"
#include "../Base/Helper.h"
#include "PostProcPS.h"
#include "BlurPS.h"

namespace CAULDRON_DX12
{
    float GaussianWeight(float x)
    {
        return expf(-.5f* x*x) / sqrtf(2.0f*XM_PI);
    }

    float GaussianArea(float x0, float x1, int samples)
    {
        float n = 0;

        for (int i = 0; i < samples; i++)
        {
            float t = (float)i / (float)(samples);
            float x = ((1.0f - t) * x0) + (t * x1);
            n += GaussianWeight(x);
        }

        return n * (x1 - x0) / (float)samples;
    }

    void GenerateGaussianWeights(int count, float *out)
    {
        // a 3 sigma width covers 99.7 of the kernel
        float delta = 3.0f / (float)count;

        out[0] = GaussianArea(0.0f, delta / 2.0f, 500) * 2.0f;

        for (int i = 1; i < count; i++)
        {
            float x = delta*i - (delta / 2.0f);
            out[i] = GaussianArea(x, x + delta, 1000);
        }
    }

    void BlurPS::OnCreate(
        Device *pDevice,
        ResourceViewHeaps      *pResourceViewHeaps,
        DynamicBufferRing      *pConstantBufferRing,
        StaticBufferPool       *pStaticBufferPool,        
        DXGI_FORMAT outFormat
    )
    {
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_pConstantBufferRing = pConstantBufferRing;
        m_outFormat = outFormat;

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

        m_directionalBlur.OnCreate(pDevice, "blur.hlsl", pResourceViewHeaps, pStaticBufferPool, 1, 1, &SamplerDesc, m_outFormat);

        // Allocate descriptors for the mip chain
        //
        for (int i = 0; i < BLURPS_MAX_MIP_LEVELS; i++)
        {
            // Horizontal pass
            //
            m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_horizontalMip[i].m_SRV);
            m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_horizontalMip[i].m_RTV);

            // Vertical pass
            //
            m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_verticalMip[i].m_SRV);
            m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_verticalMip[i].m_RTV);
        }

        /*
        float out[16];
        for (int k = 3; k <= 16; k++)
        {
            GenerateGaussianWeights(k, out);

            char str[1024];
            int ii = 0;
            ii += sprintf_s(&str[ii], 1024 - ii, "int s_lenght = %i; float s_coeffs[] = {", k);
            for (int i = 0; i < k; i++)
            {
                ii += sprintf_s(&str[ii], 1024 - ii, "%f, ", out[i]);
            }

            float r = out[0];
            for (int i = 1; i < k; i++)
                r += 2 * out[i];

            assert(r <= 1.0f && r>0.97f);


            ii += sprintf_s(&str[ii], 1024 - ii, "}; // norm = %f\n", r);

            OutputDebugStringA(str);
        }
    */
    }

    void BlurPS::OnCreateWindowSizeDependentResources(Device *pDevice, uint32_t Width, uint32_t Height, Texture *pInput, int mipCount)
    {
        m_Width = Width;
        m_Height = Height;
        m_mipCount = mipCount;
        m_pInput = pInput;

        // Create a temporary texture to hold the horizontal pass (only now we know the size of the render target we want to downsample, hence we create the temporary render target here)
        //
        m_tempBlur.InitRenderTarget(pDevice, "BlurPS::m_tempBlur", &CD3DX12_RESOURCE_DESC::Tex2D(m_outFormat, m_Width, m_Height, 1, m_mipCount, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // Create views for the mip chain
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            // Horizontal pass, from pInput to m_tempBlur
            //
            pInput->CreateSRV(0, &m_horizontalMip[i].m_SRV, i);
            m_tempBlur.CreateRTV(0, &m_horizontalMip[i].m_RTV, i);

            // Vertical pass, from m_tempBlur back to pInput
            //
            m_tempBlur.CreateSRV(0, &m_verticalMip[i].m_SRV, i);
            pInput->CreateRTV(0, &m_verticalMip[i].m_RTV, i);
        }
    }

    void BlurPS::OnDestroyWindowSizeDependentResources()
    {
        m_tempBlur.OnDestroy();
    }

    void BlurPS::OnDestroy()
    {
        m_directionalBlur.OnDestroy();
    }

    void BlurPS::Draw(ID3D12GraphicsCommandList* pCommandList, int mipLevel)
    {
        UserMarker marker(pCommandList, "BlurPS");

        // assumes all is in D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE and leaves all in that state

        SetViewportAndScissor(pCommandList, 0, 0, m_Width >> mipLevel, m_Height >> mipLevel);

        pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_tempBlur.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, mipLevel));

        // horizontal pass
        //
        {
            pCommandList->OMSetRenderTargets(1, &m_horizontalMip[mipLevel].m_RTV.GetCPU(), true, NULL);

            BlurPS::cbBlur *data;
            D3D12_GPU_VIRTUAL_ADDRESS constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(BlurPS::cbBlur), (void **)&data, &constantBuffer);
            data->dirX = 1.0f / (float)(m_Width >> mipLevel);
            data->dirY = 0.0f / (float)(m_Height >> mipLevel);
            data->mipLevel = mipLevel;
            m_directionalBlur.Draw(pCommandList, 1, &m_horizontalMip[mipLevel].m_SRV, constantBuffer);
        }

        D3D12_RESOURCE_BARRIER trans[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_tempBlur.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, mipLevel),
            CD3DX12_RESOURCE_BARRIER::Transition(m_pInput->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, mipLevel)
        };
        pCommandList->ResourceBarrier(2, trans);

        // vertical pass
        //
        {
            pCommandList->OMSetRenderTargets(1, &m_verticalMip[mipLevel].m_RTV.GetCPU(), true, NULL);

            BlurPS::cbBlur *data;
            D3D12_GPU_VIRTUAL_ADDRESS constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(BlurPS::cbBlur), (void **)&data, &constantBuffer);
            data->dirX = 0.0f / (float)(m_Width >> mipLevel);
            data->dirY = 1.0f / (float)(m_Height >> mipLevel);
            data->mipLevel = mipLevel;
            m_directionalBlur.Draw(pCommandList, 1, &m_verticalMip[mipLevel].m_SRV, constantBuffer);
        }

        pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pInput->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, mipLevel));

    }

    void BlurPS::Draw(ID3D12GraphicsCommandList* pCommandList)
    {
        for (int i = 0; i < m_mipCount; i++)
        {
            Draw(pCommandList, i);
        }
    }
}