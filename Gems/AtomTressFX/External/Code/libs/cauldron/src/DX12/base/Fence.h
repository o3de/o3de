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

#include "Device.h"

namespace CAULDRON_DX12
{
    //
    // Just a simple class that automatically increments the fence counter
    //
    class Fence
    {
    public:
        Fence();
        ~Fence();
        void OnCreate(Device *pDevice, const char* pDebugName);
        void OnDestroy();
        void IssueFence(ID3D12CommandQueue* pCommandQueue);

        // This member is useful for tracking how ahead the CPU is from the GPU
        //
        // If the fence is used once per frame, calling this function with  
        // WaitForFence(3) will make sure the CPU is no more than 3 frames ahead
        //
        void CpuWaitForFence(UINT64 olderFence);
        void GpuWaitForFence(ID3D12CommandQueue* pCommandQueue);

    private:
        HANDLE       m_hEvent;
        ID3D12Fence *m_pFence;
        UINT64       m_fenceCounter;
    };


    void GPUFlush(ID3D12Device *pDevice, ID3D12CommandQueue *pQueue);
}