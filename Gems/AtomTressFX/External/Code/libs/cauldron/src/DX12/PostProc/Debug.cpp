// AMD Cauldron code
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
#include "base/Device.h"
#include "base/StaticBufferPool.h"
#include "base/ResourceViewHeaps.h"
#include "PostProcPS.h"
#include "Debug.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void Debug::OnCreate(Device *pDevice, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool *pStaticBufferPool, DXGI_FORMAT outFormat)
    {
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

        m_debug.OnCreate(pDevice, "Debug.hlsl", pResourceViewHeaps, pStaticBufferPool, 1, 1, &SamplerDesc, outFormat);
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void Debug::OnDestroy()
    {
        m_debug.OnDestroy();
    }

    //--------------------------------------------------------------------------------------
    //
    // UpdatePipelines
    //
    //--------------------------------------------------------------------------------------
    void Debug::UpdatePipelines(DXGI_FORMAT outFormat)
    {
        m_debug.UpdatePipeline(outFormat);
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void Debug::Draw(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pDebugBufferSRV)
    {
        UserMarker marker(pCommandList, "Debug");

        m_debug.Draw(pCommandList, 1, pDebugBufferSRV, 0);
    }
}
