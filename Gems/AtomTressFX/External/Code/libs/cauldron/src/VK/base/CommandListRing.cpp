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
#include "CommandListRing.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerBackBuffer, bool compute /* = false */)
    {
        m_pDevice = pDevice;
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
            VkCommandPoolCreateInfo cmd_pool_info = {};
            cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.pNext = NULL;
            if (compute == false)
            {
                cmd_pool_info.queueFamilyIndex = pDevice->GetGraphicsQueueFamilyIndex();
            }
            else
            {
                cmd_pool_info.queueFamilyIndex = pDevice->GetComputeQueueFamilyIndex();
            }
            cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            VkResult res = vkCreateCommandPool(pDevice->GetDevice(), &cmd_pool_info, NULL, &pCBPF->m_commandPool);
            assert(res == VK_SUCCESS);

            // Create command buffers
            //
            pCBPF->m_pCommandBuffer = new VkCommandBuffer[m_commandListsPerBackBuffer];
            VkCommandBufferAllocateInfo cmd = {};
            cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmd.pNext = NULL;
            cmd.commandPool = pCBPF->m_commandPool;
            cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmd.commandBufferCount = commandListsPerBackBuffer;
            res = vkAllocateCommandBuffers(pDevice->GetDevice(), &cmd, pCBPF->m_pCommandBuffer);
            assert(res == VK_SUCCESS);

            pCBPF->m_UsedCls = 0;
        }

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
            vkFreeCommandBuffers(m_pDevice->GetDevice(), m_pCommandBuffers[a].m_commandPool, m_commandListsPerBackBuffer, m_pCommandBuffers[a].m_pCommandBuffer);

            vkDestroyCommandPool(m_pDevice->GetDevice(), m_pCommandBuffers[a].m_commandPool, NULL);
        }

        delete[] m_pCommandBuffers;
    }

    //--------------------------------------------------------------------------------------
    //
    // GetNewCommandList
    //
    //--------------------------------------------------------------------------------------
    VkCommandBuffer CommandListRing::GetNewCommandList()
    {
        VkCommandBuffer commandBuffer = m_pCurrentFrame->m_pCommandBuffer[m_pCurrentFrame->m_UsedCls++];

        assert(m_pCurrentFrame->m_UsedCls < m_commandListsPerBackBuffer); //if hit increase commandListsPerBackBuffer

        return commandBuffer;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnBeginFrame
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnBeginFrame()
    {
        m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];

        m_pCurrentFrame->m_UsedCls = 0;

        m_frameIndex++;
    }

}