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
#include "d3dx12.h"
#include "../AGS\amd_ags.h"

namespace CAULDRON_DX12
{
    //
    // Just a device class with many helper functions
    //

    class Device
    {
    public:
        Device();
        ~Device();

        void Device::OnCreate(const char *pAppName, const char *pEngine, bool bValidationEnabled, HWND hWnd);
        void OnDestroy();

        ID3D12Device *GetDevice() { return m_pDevice; }
        IDXGIAdapter *GetAdapter() { return m_pAdapter; }
        ID3D12CommandQueue *GetGraphicsQueue() { return m_pDirectQueue; }
        ID3D12CommandQueue* GetComputeQueue() { return m_pComputeQueue; }

        AGSContext *GetAGSContext() { return m_agsContext; }
        AGSGPUInfo *GetAGSGPUInfo() { return &m_agsGPUInfo; }

        bool IsFp16Supported() { return m_fp16Supported; };
        void CreatePipelineCache();
        void DestroyPipelineCache();

        void GPUFlush();

    private:
        ID3D12Device         *m_pDevice;
        IDXGIAdapter         *m_pAdapter;
        ID3D12CommandQueue   *m_pDirectQueue;
        ID3D12CommandQueue   *m_pComputeQueue;

        AGSContext           *m_agsContext = nullptr;
        AGSGPUInfo            m_agsGPUInfo = {};
        bool                  m_fp16Supported = false;
    };
}
