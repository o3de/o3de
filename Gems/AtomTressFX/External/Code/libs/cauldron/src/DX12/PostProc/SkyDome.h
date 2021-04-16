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
#pragma once

#include "PostProcPS.h"
#include "Base/Texture.h"
#include "Base/UploadHeap.h"
#include "Base/StaticBufferPool.h"
#include "Base/DynamicBufferRing.h"

namespace CAULDRON_DX12
{
    class SkyDome
    {
    public:
        void OnCreate(Device* pDevice, UploadHeap* pUploadHeap, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing, StaticBufferPool  *pStaticBufferPool, char *pDiffuseCubemap, char *pSpecularCubemap, DXGI_FORMAT outFormat, uint32_t sampleDescCount);
        void OnDestroy();
        void Draw(ID3D12GraphicsCommandList* pCommandList, XMMATRIX& invViewProj);
        void GenerateDiffuseMapFromEnvironmentMap();

        void SetDescriptorDiff(uint32_t textureIndex, CBV_SRV_UAV *pTextureTable, uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc);
        void SetDescriptorSpec(uint32_t textureIndex, CBV_SRV_UAV *pTextureTable, uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc);

    private:
        Texture m_CubeDiffuseTexture;
        Texture m_CubeSpecularTexture;        

        CBV_SRV_UAV m_CubeSpecularTextureSRV;

        PostProcPS  m_skydome;

        DynamicBufferRing *m_pDynamicBufferRing = NULL;
    };
}
