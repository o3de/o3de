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
    // This class manages command allocators and command lists. 
    // For each backbuffer creates a command list allocator and creates <commandListsPerframe> command lists. 
    //
    class CommandListRing
    {
    public:
        void OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerframe, D3D12_COMMAND_QUEUE_DESC queueDesc);
        void OnDestroy();
        void OnBeginFrame();
        ID3D12GraphicsCommandList2 *GetNewCommandList();
        ID3D12CommandAllocator *GetAllocator() { return m_pCurrentFrame->m_pCommandAllocator; }

    private:
        uint32_t m_frameIndex;
        uint32_t m_numberOfAllocators;
        uint32_t m_commandListsPerBackBuffer;

        struct CommandBuffersPerFrame
        {
            ID3D12CommandAllocator       *m_pCommandAllocator;
            ID3D12GraphicsCommandList2    **m_ppCommandList;
            uint32_t m_UsedCls;
        } *m_pCommandBuffers, *m_pCurrentFrame;
    };
}
