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
#include "Fence.h"
#include "Misc/Error.h"
#include "Base/Helper.h"

namespace CAULDRON_DX12
{
    Fence::Fence()
    {
        m_hEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    }

    Fence::~Fence()
    {
        CloseHandle(m_hEvent);
    }

    void Fence::OnCreate(Device *pDevice, const char* pDebugName)
    {
        m_fenceCounter = 0;
        ThrowIfFailed(pDevice->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
        SetName(m_pFence, pDebugName);
    }

    void Fence::OnDestroy()
    {
        m_pFence->Release();
    }

    void Fence::IssueFence(ID3D12CommandQueue* pCommandQueue)
    {
        m_fenceCounter++;
        ThrowIfFailed(pCommandQueue->Signal(m_pFence, m_fenceCounter));
    }

    // This member is useful for tracking how ahead the CPU is from the GPU
    //
    // If the fence is used once per frame, calling this function with  
    // WaitForFence(3) will make sure the CPU is no more than 3 frames ahead
    //
    void Fence::CpuWaitForFence(UINT64 olderFence)
    {
        if (m_fenceCounter > olderFence)
        {
            UINT64 valueToWaitFor = m_fenceCounter - olderFence;

            if (m_pFence->GetCompletedValue() <= valueToWaitFor)
            {
                ThrowIfFailed(m_pFence->SetEventOnCompletion(valueToWaitFor, m_hEvent));
                WaitForSingleObject(m_hEvent, INFINITE);
            }
        }
    }

    void Fence::GpuWaitForFence(ID3D12CommandQueue* pCommandQueue)
    {
        ThrowIfFailed(pCommandQueue->Wait(m_pFence, m_fenceCounter));
    }

}