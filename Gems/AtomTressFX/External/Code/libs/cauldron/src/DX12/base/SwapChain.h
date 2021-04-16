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
#include <dxgi1_4.h>
#include "Fence.h"
#include "Device.h"
#include "FreeSync2.h"

namespace CAULDRON_DX12
{
    class SwapChain
    {
    public:
        void OnCreate(Device *pDevice, uint32_t numberBackBuffers, HWND hWnd, DisplayModes displayMode);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(uint32_t dwWidth, uint32_t dwHeight, bool bVSyncOn, DisplayModes displayMode);
        void OnDestroyWindowSizeDependentResources();

        void SetFullScreen(bool fullscreen);

        void Present();
        ID3D12Resource *GetCurrentBackBufferResource();
        D3D12_CPU_DESCRIPTOR_HANDLE *GetCurrentBackBufferRTV();
        void WaitForSwapChain();
        void CreateRTV();
        DXGI_FORMAT GetFormat();

    private:
        HWND m_hWnd = NULL;
        uint32_t m_BackBufferCount = 0;

        ID3D12Device *m_pDevice = NULL;
        IDXGIFactory4 *m_pFactory = NULL;
        IDXGISwapChain3 *m_pSwapChain = NULL;

        DXGI_FORMAT m_swapChainFormat = DXGI_FORMAT_UNKNOWN;

        Fence m_swapChainFence;

        ID3D12CommandQueue*    m_pDirectQueue;

        ID3D12DescriptorHeap * m_RTVHeaps;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_CPUView;

        DXGI_SWAP_CHAIN_DESC1 m_descSwapChain = {};

        bool m_bVSyncOn = false;
    };
}
