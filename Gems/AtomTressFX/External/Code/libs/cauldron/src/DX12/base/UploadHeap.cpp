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
#include "UploadHeap.h"
#include "Misc/Misc.h"
#include "Base/Helper.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::OnCreate(Device *pDevice, SIZE_T uSize)
    {
        m_pDevice = pDevice;
        m_pCommandQueue = pDevice->GetGraphicsQueue();

        // Create command list and allocators 

        pDevice->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
        SetName(m_pCommandAllocator, "UploadHeap::m_pCommandAllocator");
        pDevice->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList));
        SetName(m_pCommandList, "UploadHeap::m_pCommandList");

        // Create buffer to suballocate

        ThrowIfFailed(
            pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(uSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pUploadHeap)
            )
        );

        ThrowIfFailed(m_pUploadHeap->Map(0, NULL, (void**)&m_pDataBegin));

        m_pDataCur = m_pDataBegin;
        m_pDataEnd = m_pDataBegin + m_pUploadHeap->GetDesc().Width;

        m_fenceValue = 0;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::OnDestroy()
    {
        m_pUploadHeap->Release();

        m_pCommandList->Release();
        m_pCommandAllocator->Release();
    }

    //--------------------------------------------------------------------------------------
    //
    // SuballocateFromUploadHeap
    //
    //--------------------------------------------------------------------------------------
    UINT8* UploadHeap::Suballocate(SIZE_T uSize, UINT64 uAlign)
    {
        m_pDataCur = reinterpret_cast<UINT8*>(AlignOffset(reinterpret_cast<SIZE_T>(m_pDataCur), SIZE_T(uAlign)));

        // return NULL if we ran out of space in the heap
        if (m_pDataCur >= m_pDataEnd || m_pDataCur + uSize >= m_pDataEnd)
        {
            return NULL;
        }

        UINT8* pRet = m_pDataCur;
        m_pDataCur += uSize;
        return pRet;
    }

    //--------------------------------------------------------------------------------------
    //
    // FlushAndFinish
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::FlushAndFinish()
    {
        // Close & submit

        ThrowIfFailed(m_pCommandList->Close());
        m_pCommandQueue->ExecuteCommandLists(1, CommandListCast(&m_pCommandList));

        // Make sure it's been processed by the GPU
        m_pDevice->GPUFlush();

        // Reset so it can be reused
        m_pCommandAllocator->Reset();
        m_pCommandList->Reset(m_pCommandAllocator, nullptr);

        m_pDataCur = m_pDataBegin;
    }
}