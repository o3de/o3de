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
#include <mutex>
#include "Device.h"


namespace CAULDRON_DX12
{
    // Simulates DX11 style static buffers. For dynamic buffers please see 'DynamicBufferRingDX12.h'
    //
    // This class allows suballocating small chuncks of memory from a huge buffer that is allocated on creation 
    // This class is specialized in vertex buffers.
    //

    class StaticBufferPool
    {
    public:
        void OnCreate(Device *pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name);
        void OnDestroy();

        bool AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void **pData, D3D12_GPU_VIRTUAL_ADDRESS *pBufferLocation, uint32_t *pSize);
        bool AllocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void *pInitData, D3D12_GPU_VIRTUAL_ADDRESS *pBufferLocation, uint32_t *pSize);

        bool AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void **pData, D3D12_VERTEX_BUFFER_VIEW *pView);
        bool AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void **pData, D3D12_INDEX_BUFFER_VIEW *pIndexView);
        bool AllocConstantBuffer(uint32_t size, void **pData, D3D12_CONSTANT_BUFFER_VIEW_DESC  *pViewDesc);

        bool AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void *pInitData, D3D12_VERTEX_BUFFER_VIEW *pOut);
        bool AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void *pInitData, D3D12_INDEX_BUFFER_VIEW *pOut);
        bool AllocConstantBuffer(uint32_t size, void *pData, D3D12_CONSTANT_BUFFER_VIEW_DESC  *pViewDesc);

        void UploadData(ID3D12GraphicsCommandList *pCmdList);
        void FreeUploadHeap();
        
        ID3D12Resource *GetResource();

    private:
        Device          *m_pDevice = nullptr;

        std::mutex       m_mutex = {};

        bool             m_bUseVidMem = true;

        char            *m_pData = nullptr;
        uint32_t         m_memInit = 0;
        uint32_t         m_memOffset = 0;
        uint32_t         m_totalMemSize = 0;

        ID3D12Resource  *m_pMemBuffer = nullptr;
        ID3D12Resource  *m_pSysMemBuffer = nullptr;
        ID3D12Resource  *m_pVidMemBuffer = nullptr;
    };
}

