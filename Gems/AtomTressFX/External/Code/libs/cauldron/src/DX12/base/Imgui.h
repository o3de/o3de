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

#include "ResourceViewHeaps.h"
#include "DynamicBufferRing.h"
#include "CommandListRing.h"
#include "UploadHeap.h"
#include "../imgui\imgui.h"

namespace CAULDRON_DX12
{
    // This is the rendering backend for the excellent ImGUI library.

    class ImGUI
    {
    public:
        void OnCreate(Device *pDevice, UploadHeap *pUploadHeap, ResourceViewHeaps *pHeaps, DynamicBufferRing *pConstantBufferRing, DXGI_FORMAT outFormat);
        void OnDestroy();
        void UpdatePipeline(DXGI_FORMAT outFormat);
        void Draw(ID3D12GraphicsCommandList *pCmdLst);

    private:
        Device                    *m_pDevice = nullptr;
        ResourceViewHeaps         *m_pResourceViewHeaps = nullptr;
        DynamicBufferRing         *m_pConstBuf = nullptr;

        ID3D12Resource            *m_pTexture2D = nullptr;
        ID3D12PipelineState       *m_pPipelineState = nullptr;
        ID3D12RootSignature       *m_pRootSignature = nullptr;

        D3D12_SHADER_BYTECODE      m_shaderVert, m_shaderPixel;

        CBV_SRV_UAV                m_pTextureSRV;
    };
}
