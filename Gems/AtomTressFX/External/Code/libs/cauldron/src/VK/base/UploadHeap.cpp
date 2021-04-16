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
#include "UploadHeap.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::OnCreate(Device *pDevice, SIZE_T uSize)
    {
        m_pDevice = pDevice;

        VkResult res;

        // Create command list and allocators 
        {
            VkCommandPoolCreateInfo cmd_pool_info = {};
            cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.pNext = NULL;
            cmd_pool_info.queueFamilyIndex = m_pDevice->GetGraphicsQueueFamilyIndex();
            cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            res = vkCreateCommandPool(m_pDevice->GetDevice(), &cmd_pool_info, NULL, &m_commandPool);
            assert(res == VK_SUCCESS);

            VkCommandBufferAllocateInfo cmd = {};
            cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmd.pNext = NULL;
            cmd.commandPool = m_commandPool;
            cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmd.commandBufferCount = 1;
            res = vkAllocateCommandBuffers(m_pDevice->GetDevice(), &cmd, &m_pCommandBuffer);
            assert(res == VK_SUCCESS);
        }

        // Create buffer to suballocate
        {
            VkBufferCreateInfo buffer_info = {};
            buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size = uSize;
            buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            res = vkCreateBuffer(m_pDevice->GetDevice(), &buffer_info, NULL, &m_buffer);
            assert(res == VK_SUCCESS);

            VkMemoryRequirements mem_reqs;
            vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), m_buffer, &mem_reqs);

            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_reqs.size;
            alloc_info.memoryTypeIndex = 0;

            bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                &alloc_info.memoryTypeIndex);
            assert(pass && "No mappable, coherent memory");

            res = vkAllocateMemory(m_pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemory);
            assert(res == VK_SUCCESS);

            res = vkBindBufferMemory(m_pDevice->GetDevice(), m_buffer, m_deviceMemory, 0);
            assert(res == VK_SUCCESS);

            res = vkMapMemory(m_pDevice->GetDevice(), m_deviceMemory, 0, mem_reqs.size, 0, (void **)&m_pDataBegin);
            assert(res == VK_SUCCESS);

            m_pDataCur = m_pDataBegin;
            m_pDataEnd = m_pDataBegin + mem_reqs.size;
        }

        // Create fence
        {
            VkFenceCreateInfo fence_ci;
            fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_ci.pNext = NULL;
            fence_ci.flags = 0;

            res = vkCreateFence(m_pDevice->GetDevice(), &fence_ci, NULL, &m_fence);
            assert(res == VK_SUCCESS);
        }

        // Begin Command Buffer
        {
            VkCommandBufferBeginInfo cmd_buf_info;
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = NULL;
            cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            cmd_buf_info.pInheritanceInfo = NULL;

            res = vkBeginCommandBuffer(m_pCommandBuffer, &cmd_buf_info);
            assert(res == VK_SUCCESS);
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::OnDestroy()
    {
        vkUnmapMemory(m_pDevice->GetDevice(), m_deviceMemory);
        vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
        vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);

        vkFreeCommandBuffers(m_pDevice->GetDevice(), m_commandPool, 1, &m_pCommandBuffer);
        vkDestroyCommandPool(m_pDevice->GetDevice(), m_commandPool, NULL);

        vkDestroyFence(m_pDevice->GetDevice(), m_fence, NULL);
    }

    //--------------------------------------------------------------------------------------
    //
    // SuballocateFromUploadHeap
    //
    //--------------------------------------------------------------------------------------
    UINT8* UploadHeap::Suballocate(SIZE_T uSize, UINT64 uAlign)
    {
        m_pDataCur = reinterpret_cast<UINT8*>(AlignOffset(reinterpret_cast<SIZE_T>(m_pDataCur), uAlign));
        uSize = AlignOffset(uSize, uAlign);

        // flush operations if we ran out of space in the heap

        if ((m_pDataCur >= m_pDataEnd) || (m_pDataCur + uSize >= m_pDataEnd))
        {
            return NULL;
        }

        UINT8* pRet = m_pDataCur;
        m_pDataCur += uSize;
        return pRet;
    }

    void UploadHeap::Flush()
    {
        VkResult res;

        VkMappedMemoryRange range[1] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = m_deviceMemory;
        range[0].size = m_pDataCur - m_pDataBegin;
        res = vkFlushMappedMemoryRanges(m_pDevice->GetDevice(), 1, range);
        assert(res == VK_SUCCESS);
    }

    //--------------------------------------------------------------------------------------
    //
    // FlushAndFinish
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::FlushAndFinish()
    {
        VkResult res;

        Flush();

        // Close 
        res = vkEndCommandBuffer(m_pCommandBuffer);
        assert(res == VK_SUCCESS);

        // Submit

        const VkCommandBuffer cmd_bufs[] = { m_pCommandBuffer };
        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = cmd_bufs;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = NULL;

        res = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submit_info, m_fence);
        assert(res == VK_SUCCESS);

        // Make sure it's been processed by the GPU

        res = vkWaitForFences(m_pDevice->GetDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
        assert(res == VK_SUCCESS);

        vkResetFences(m_pDevice->GetDevice(), 1, &m_fence);

        // Reset so it can be reused
        VkCommandBufferBeginInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = NULL;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        cmd_buf_info.pInheritanceInfo = NULL;

        res = vkBeginCommandBuffer(m_pCommandBuffer, &cmd_buf_info);
        assert(res == VK_SUCCESS);

        m_pDataCur = m_pDataBegin;
    }
}