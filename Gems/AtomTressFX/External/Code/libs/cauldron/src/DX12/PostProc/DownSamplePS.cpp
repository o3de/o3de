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
#include "Base/UploadHeap.h"
#include "Base/Texture.h"
#include "Base/Imgui.h"
#include "Base/Helper.h"

#include "PostProcPS.h"
#include "DownSamplePS.h"

namespace CAULDRON_DX12
{
    void DownSamplePS::OnCreate(
        Device *pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pConstantBufferRing,
        StaticBufferPool  *pStaticBufferPool,
        DXGI_FORMAT outFormat
    )
    {
        m_pDevice = pDevice;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_pConstantBufferRing = pConstantBufferRing;
        m_outFormat = outFormat;

        // Use helper class to create the fullscreen pass
        //

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

        m_downscale.OnCreate(pDevice, "DownSamplePS.hlsl", m_pResourceViewHeaps, m_pStaticBufferPool, 1, 1, &SamplerDesc, m_outFormat);

        // Allocate descriptors for the mip chain
        //
        for (int i = 0; i < DOWNSAMPLEPS_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_mip[i].m_SRV);
            m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_mip[i].m_RTV);
        }

    }

    void DownSamplePS::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture *pInput, int mipCount)
    {
        m_Width = Width;
        m_Height = Height;
        m_mipCount = mipCount;
        m_pInput = pInput;

        m_result.InitRenderTarget(m_pDevice, "DownSamplePS::m_result", &CD3DX12_RESOURCE_DESC::Tex2D(m_outFormat, m_Width >> 1, m_Height >> 1, 1, mipCount, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // Create views for the mip chain
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            // source 
            //
            if (i == 0)
            {
                pInput->CreateSRV(0, &m_mip[i].m_SRV, 0);
            }
            else
            {
                m_result.CreateSRV(0, &m_mip[i].m_SRV, i - 1);
            }

            // destination 
            //
            m_result.CreateRTV(0, &m_mip[i].m_RTV, i);
        }
    }

    void DownSamplePS::OnDestroyWindowSizeDependentResources()
    {
        m_result.OnDestroy();
    }

    void DownSamplePS::OnDestroy()
    {
        m_downscale.OnDestroy();
    }

    void DownSamplePS::Draw(ID3D12GraphicsCommandList* pCommandList)
    {
        UserMarker marker(pCommandList, "DownSamplePS");

        // downsample
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            pCommandList->OMSetRenderTargets(1, &m_mip[i].m_RTV.GetCPU(), true, NULL);
            SetViewportAndScissor(pCommandList, 0, 0, m_Width >> (i + 1), m_Height >> (i + 1));

            cbDownscale *data;
            D3D12_GPU_VIRTUAL_ADDRESS constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(cbDownscale), (void **)&data, &constantBuffer);
            data->invWidth = 1.0f / (float)(m_Width >> i);
            data->invHeight = 1.0f / (float)(m_Height >> i);
            data->mipLevel = i;

            if (i > 0)
            {
                pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_result.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, i - 1));
            }

            pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_result.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, i));

            m_downscale.Draw(pCommandList, 1, &m_mip[i].m_SRV, constantBuffer);
        }

        pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_result.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, m_mipCount - 1));
    }

    void DownSamplePS::Gui()
    {
        bool opened = true;
        ImGui::Begin("DownSamplePS", &opened);

        for (int i = 0; i < m_mipCount; i++)
        {
            ImGui::Image((ImTextureID)&m_mip[i].m_SRV, ImVec2(320 / 2, 180 / 2));
        }

        ImGui::End();
    }
}
