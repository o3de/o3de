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
#include "Device.h"
#include "../..\common\Misc/Misc.h"
#include "Helper.h"
#include "CommandListRing.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerBackBuffer, D3D12_COMMAND_QUEUE_DESC queueDesc)
    {
        m_numberOfAllocators = numberOfBackBuffers;
        m_commandListsPerBackBuffer = commandListsPerBackBuffer;

        m_pCommandBuffers = new CommandBuffersPerFrame[m_numberOfAllocators]();

        // Create command allocators, for each frame in flight we wannt to have a single Command Allocator, and <commandListsPerBackBuffer> command buffers
        //
        for (uint32_t a = 0; a < m_numberOfAllocators; a++)
        {
            CommandBuffersPerFrame *pCBPF = &m_pCommandBuffers[a];

            // Create allocator
            //
            ThrowIfFailed(pDevice->GetDevice()->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&pCBPF->m_pCommandAllocator)));
            SetName(pCBPF->m_pCommandAllocator, format("CommandAllocator %i", a));

            // Create command buffers
            //
            pCBPF->m_ppCommandList = new ID3D12GraphicsCommandList2*[m_commandListsPerBackBuffer];
            for (uint32_t i = 0; i < m_commandListsPerBackBuffer; i++)
            {
                ThrowIfFailed(pDevice->GetDevice()->CreateCommandList(0, queueDesc.Type, pCBPF->m_pCommandAllocator, nullptr, IID_PPV_ARGS(&pCBPF->m_ppCommandList[i])));
                pCBPF->m_ppCommandList[i]->Close();
                SetName(pCBPF->m_ppCommandList[i], format("CommandList %i, Allocator %i", i, a));
            }
            pCBPF->m_UsedCls = 0;
        }

        // Closing all the command buffers so we can call reset on them the first time we use them, otherwise the runtime would show a warning. 
        //
        ID3D12CommandQueue* queue = pDevice->GetGraphicsQueue();
        if (queueDesc.Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            queue = pDevice->GetComputeQueue();
        }

        for (uint32_t a = 0; a < m_numberOfAllocators; a++)
        {
            queue->ExecuteCommandLists(m_commandListsPerBackBuffer, (ID3D12CommandList *const *)m_pCommandBuffers[a].m_ppCommandList);
        }        

        pDevice->GPUFlush();

        m_frameIndex = 0;
        m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];
        m_frameIndex++;
        m_pCurrentFrame->m_UsedCls = 0;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnDestroy()
    {
        //release and delete command allocators
        for (uint32_t a = 0; a < m_numberOfAllocators; a++)
        {
            m_pCommandBuffers[a].m_pCommandAllocator->Release();

            for (uint32_t i = 0; i < m_commandListsPerBackBuffer; i++)
                m_pCommandBuffers[a].m_ppCommandList[i]->Release();
        }

        delete[] m_pCommandBuffers;
    }

    //--------------------------------------------------------------------------------------
    //
    // GetNewCommandList
    //
    //--------------------------------------------------------------------------------------
    ID3D12GraphicsCommandList2 *CommandListRing::GetNewCommandList()
    {      
        ID3D12GraphicsCommandList2 *pCL = m_pCurrentFrame->m_ppCommandList[m_pCurrentFrame->m_UsedCls++];
                
        assert(m_pCurrentFrame->m_UsedCls < m_commandListsPerBackBuffer); //if hit increase commandListsPerBackBuffer

        // reset command list and assign current allocator
        ThrowIfFailed(pCL->Reset(m_pCurrentFrame->m_pCommandAllocator, nullptr));

        return pCL;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnBeginFrame
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnBeginFrame()
    {
        m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];

        //reset the allocator
        ThrowIfFailed(m_pCurrentFrame->m_pCommandAllocator->Reset());

        m_pCurrentFrame->m_UsedCls = 0;

        m_frameIndex++;
    }
}