// AMD AMDUtils code
// 
// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
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
#include "SkyDome.h"

namespace CAULDRON_DX12
{
    void SkyDome::OnCreate(Device* pDevice, UploadHeap* pUploadHeap, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing, StaticBufferPool  *pStaticBufferPool, char *pDiffuseCubemap, char *pSpecularCubemap, DXGI_FORMAT outFormat, uint32_t sampleDescCount)
    {
        m_pDynamicBufferRing = pDynamicBufferRing;
        
        m_CubeDiffuseTexture.InitFromFile(pDevice, pUploadHeap, pDiffuseCubemap, true);
        m_CubeSpecularTexture.InitFromFile(pDevice, pUploadHeap, pSpecularCubemap, true);
        
        pUploadHeap->FlushAndFinish();

        D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};
        pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_CubeSpecularTextureSRV);
        SetDescriptorSpec(0, &m_CubeSpecularTextureSRV, 0, &SamplerDesc);

        D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        m_skydome.OnCreate(pDevice, "SkyDome.hlsl", pResourceViewHeaps, pStaticBufferPool, 1, 1, &SamplerDesc, outFormat, sampleDescCount);
    }

    void SkyDome::OnDestroy()
    {
        m_skydome.OnDestroy();

        m_CubeDiffuseTexture.OnDestroy();
        m_CubeSpecularTexture.OnDestroy();        
    }

    void SkyDome::SetDescriptorDiff(uint32_t textureIndex, CBV_SRV_UAV *pTextureTable, uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        m_CubeDiffuseTexture.CreateCubeSRV(textureIndex, pTextureTable);

        // diffuse env map sampler
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    void SkyDome::SetDescriptorSpec(uint32_t textureIndex, CBV_SRV_UAV *pTextureTable, uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        m_CubeSpecularTexture.CreateCubeSRV(textureIndex, pTextureTable);

        // specular env map sampler
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    void SkyDome::Draw(ID3D12GraphicsCommandList* pCommandList, XMMATRIX& invViewProj)
    {
        UserMarker marker(pCommandList, "Skydome");

        XMMATRIX *cbPerDraw;
        D3D12_GPU_VIRTUAL_ADDRESS constantBuffer;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(XMMATRIX), (void **)&cbPerDraw, &constantBuffer);
        *cbPerDraw = invViewProj;

        m_skydome.Draw(pCommandList, 1, &m_CubeSpecularTextureSRV, constantBuffer);
    }


    void SkyDome::GenerateDiffuseMapFromEnvironmentMap()
    {

    }
}
