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
#include "StaticConstantBufferPool.h"
#include "Misc/Misc.h"

namespace CAULDRON_DX12
{
#define ALIGN(a) ((a + 255) & ~255)

    void StaticConstantBufferPool::OnCreate(Device* pDevice, uint32_t totalMemSize, ResourceViewHeaps *pHeaps, uint32_t cbvEntriesSize, bool bUseVidMem)
    {
        m_pDevice = pDevice;

        m_totalMemSize = totalMemSize;
        m_memOffset = 0;
        m_pData = NULL;
        m_bUseVidMem = bUseVidMem;

        m_cbvEntriesSize = cbvEntriesSize;
        m_cbvOffset = 0;
        m_pCBVDesc = new D3D12_CONSTANT_BUFFER_VIEW_DESC[cbvEntriesSize];

        if (bUseVidMem)
        {
            ThrowIfFailed(
                pDevice->GetDevice()->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(totalMemSize),
                    D3D12_RESOURCE_STATE_COMMON,
                    nullptr,
                    IID_PPV_ARGS(&m_pVidMemBuffer))
            );
            SetName(m_pVidMemBuffer, "StaticConstantBufferPoolDX12::m_pVidMemBuffer");
        }

        ThrowIfFailed(
            pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(totalMemSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pSysMemBuffer))
        );
        SetName(m_pSysMemBuffer, "StaticConstantBufferPoolDX12::m_pSysMemBuffer");

        m_pSysMemBuffer->Map(0, NULL, reinterpret_cast<void**>(&m_pData));
    }

    void StaticConstantBufferPool::OnDestroy()
    {
        if (m_bUseVidMem)
        {
            m_pVidMemBuffer->Release();
        }

        if(m_pSysMemBuffer)
        {
            m_pSysMemBuffer->Release();
            m_pSysMemBuffer = nullptr;
        }

        delete(m_pCBVDesc);
    }

    bool StaticConstantBufferPool::AllocConstantBuffer(uint32_t size, void **pData, uint32_t *pIndex)
    {
        size = ALIGN(size);
        if (m_memOffset + size >= m_totalMemSize)
        {
            Trace("Ran out of mem for 'static' buffers, please increase the allocated size\n");
            return false;
        }

        *pData = (void *)(m_pData + m_memOffset);

        m_pCBVDesc[m_cbvOffset].SizeInBytes = size;
        m_pCBVDesc[m_cbvOffset].BufferLocation = m_memOffset + ((m_bUseVidMem) ? m_pVidMemBuffer->GetGPUVirtualAddress() : m_pSysMemBuffer->GetGPUVirtualAddress());

        //returning an index allows to create more CBV for a constant buffer, this is useful when packing CBV into tables
        *pIndex = m_cbvOffset;

        m_memOffset += size;
        m_cbvOffset += 1;

        assert(m_memOffset < m_totalMemSize);
        assert(m_cbvOffset < m_cbvEntriesSize);

        return true;
    }

    bool StaticConstantBufferPool::CreateCBV(uint32_t index, int srvOffset, CBV_SRV_UAV *pCBV)
    {
        m_pDevice->GetDevice()->CreateConstantBufferView(&m_pCBVDesc[index], pCBV->GetCPU(srvOffset));
        return true;
    }

    void StaticConstantBufferPool::UploadData(ID3D12GraphicsCommandList *pCmdList)
    {
        if (m_bUseVidMem)
        {
            m_pSysMemBuffer->Unmap(0, nullptr);

            pCmdList->CopyResource(m_pVidMemBuffer, m_pSysMemBuffer);

            // With 'dynamic resources' we can use a same resource to hold Constant, Index and Vertex buffers.
            // That is because we dont need to use a transition.
            //
            // With static buffers though we need to transition them and we only have 2 options
            //      1) D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
            //      2) D3D12_RESOURCE_STATE_INDEX_BUFFER
            // Because we need to transition the whole buffer we cant have now Index buffers to share the 
            // same resource with the Vertex or Constant buffers. Hence is why we need separate classes.
            // For Index and Vertex buffers we *could* use the same resource, but index buffers need their own resource.
            // Please note that in the interest of clarity vertex buffers and constant buffers have been split into two different classes though
            pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pVidMemBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
        }
    }

    void StaticConstantBufferPool::FreeUploadHeap()
    {
        m_pSysMemBuffer->Release();
    }
}
