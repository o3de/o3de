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

#include <d3d12.h>
#include "Base/StaticBufferPool.h"
#include "Base/ResourceViewHeaps.h"

namespace CAULDRON_DX12
{
    class PostProcPS
    {
    public:
        void OnCreate(
            Device *pDevice,
            const std::string &shaderFilename,
            ResourceViewHeaps *pResourceViewHeaps,
            StaticBufferPool *pStaticBufferPool,
            uint32_t dwSRVTableSize,
            uint32_t dwStaticSamplersCount,
            D3D12_STATIC_SAMPLER_DESC *pStaticSamplers,
            DXGI_FORMAT outFormat,
            uint32_t psoSampleDescCount = 1,
            D3D12_BLEND_DESC *pBlendDesc = NULL,
            D3D12_DEPTH_STENCIL_DESC *pDepthStencilDesc = NULL,
            uint32_t numRenderTargets = 1
        );
        void OnDestroy();

        void UpdatePipeline(DXGI_FORMAT outFormat, D3D12_BLEND_DESC *pBlendDesc = NULL, D3D12_DEPTH_STENCIL_DESC *pDepthStencilDesc = NULL, uint32_t psoSampleDescCount = 1, uint32_t numRenderTargets = 1);
        void Draw(ID3D12GraphicsCommandList* pCommandList, uint32_t dwSRVTableSize, CBV_SRV_UAV *pSRVTable, D3D12_GPU_VIRTUAL_ADDRESS constantBuffer);

    private:
        ResourceViewHeaps           *m_pHeaps;
        Device                      *m_pDevice;

        D3D12_VERTEX_BUFFER_VIEW     m_verticesView;

        ResourceViewHeaps           *m_pResourceViewHeaps;

        ID3D12RootSignature         *m_pRootSignature;
        ID3D12PipelineState         *m_pPipeline = NULL;
        D3D12_SHADER_BYTECODE        m_shaderVert, m_shaderPixel;
    };
}

