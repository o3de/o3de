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
#include "ResourceViewHeaps.h"

namespace CAULDRON_DX12
{
    // Simulates DX11 style static buffers. For dynamic buffers please see 'DynamicBufferRingDX12.h'
    // 
    // This class allows suballocating small chuncks of memory from a huge buffer that is allocated on creation 
    // This class is specialized in constant buffers.
    //
    class StaticConstantBufferPool
    {
    public:
        void OnCreate(Device* pDevice, uint32_t totalMemSize, ResourceViewHeaps *pHeaps, uint32_t cbvEntriesSize, bool bUseVidMem);
        void OnDestroy();
        bool AllocConstantBuffer(uint32_t size, void **pData, uint32_t *pIndex);
        bool CreateCBV(uint32_t index, int srvOffset, CBV_SRV_UAV *pCBV);
        void UploadData(ID3D12GraphicsCommandList *pCmdList);
        void FreeUploadHeap();

    private:
        Device          *m_pDevice;
        ID3D12Resource  *m_pMemBuffer;
        ID3D12Resource  *m_pSysMemBuffer;
        ID3D12Resource  *m_pVidMemBuffer;

        char            *m_pData;
        uint32_t         m_memOffset;
        uint32_t         m_totalMemSize;

        uint32_t         m_cbvOffset;
        uint32_t         m_cbvEntriesSize;

        D3D12_CONSTANT_BUFFER_VIEW_DESC *m_pCBVDesc;

        bool            m_bUseVidMem;
    };
}

