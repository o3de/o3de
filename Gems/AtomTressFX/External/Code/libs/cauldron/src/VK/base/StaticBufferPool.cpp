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
#include "StaticBufferPool.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{
    VkResult StaticBufferPool::OnCreate(Device *pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name)
    {
        VkResult res;
        m_pDevice = pDevice;

        m_totalMemSize = totalMemSize;
        m_memOffset = 0;
        m_pData = NULL;
        m_bUseVidMem = bUseVidMem;

#ifdef USE_VMA
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = m_totalMemSize;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bUseVidMem)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocInfo.pUserData = (void*)name;

        res = vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_bufferAlloc, nullptr);
        assert(res == VK_SUCCESS);
        res = vmaMapMemory(pDevice->GetAllocator(), m_bufferAlloc, (void **)&m_pData);
        assert(res == VK_SUCCESS);
#else
        // create the buffer, allocate it in SYSTEM memory, bind it and map it

        VkBufferCreateInfo buf_info = {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = NULL;
        buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bUseVidMem)
            buf_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_info.size = m_totalMemSize;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = NULL;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;
        res = vkCreateBuffer(m_pDevice->GetDevice(), &buf_info, NULL, &m_buffer);
        assert(res == VK_SUCCESS);

        // allocate the buffer in system memory

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), m_buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_reqs.size;

        bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &alloc_info.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        res = vkAllocateMemory(m_pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemory);
        assert(res == VK_SUCCESS);

        // bind buffer

        res = vkBindBufferMemory(m_pDevice->GetDevice(), m_buffer, m_deviceMemory, 0);
        assert(res == VK_SUCCESS);

        // Map it and leave it mapped. This is fine for Win10 and Win7.

        res = vkMapMemory(m_pDevice->GetDevice(), m_deviceMemory, 0, mem_reqs.size, 0, (void **)&m_pData);
        assert(res == VK_SUCCESS);
#endif

        if (m_bUseVidMem)
        {
#ifdef USE_VMA
            VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = m_totalMemSize;
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            allocInfo.pUserData = (void*)name;

            vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &m_bufferVid, &m_bufferAllocVid, nullptr);
#else

            // create the buffer, allocate it in VIDEO memory and bind it 

            VkBufferCreateInfo buf_info = {};
            buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_info.pNext = NULL;
            buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            buf_info.size = m_totalMemSize;
            buf_info.queueFamilyIndexCount = 0;
            buf_info.pQueueFamilyIndices = NULL;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            buf_info.flags = 0;
            res = vkCreateBuffer(m_pDevice->GetDevice(), &buf_info, NULL, &m_bufferVid);
            assert(res == VK_SUCCESS);

            // allocate the buffer in VIDEO memory

            VkMemoryRequirements mem_reqs;
            vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), m_bufferVid, &mem_reqs);

            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.pNext = NULL;
            alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            alloc_info.allocationSize = mem_reqs.size;

            bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &alloc_info.memoryTypeIndex);
            assert(pass && "No mappable, coherent memory");

            res = vkAllocateMemory(m_pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemoryVid);
            assert(res == VK_SUCCESS);

            // bind buffer

            res = vkBindBufferMemory(m_pDevice->GetDevice(), m_bufferVid, m_deviceMemoryVid, 0);
            assert(res == VK_SUCCESS);
#endif
        }

        return res;
    }

    void StaticBufferPool::OnDestroy()
    {
        if (m_bUseVidMem)
        {
#ifdef USE_VMA
            vmaDestroyBuffer(m_pDevice->GetAllocator(), m_bufferVid, m_bufferAllocVid);
#else
            vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemoryVid, NULL);
            vkDestroyBuffer(m_pDevice->GetDevice(), m_bufferVid, NULL);
#endif

        }

        if (m_buffer != VK_NULL_HANDLE)
        {
#ifdef USE_VMA
            vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
            vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
            vkUnmapMemory(m_pDevice->GetDevice(), m_deviceMemory);
            vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
            vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);
#endif
            m_buffer = VK_NULL_HANDLE;
        }
    }


    bool StaticBufferPool::AllocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut)
    {
        uint32_t size = (uint32_t)AlignOffset(numbeOfElements* strideInBytes, 256);
        assert(m_memOffset + size < m_totalMemSize);

        *pData = (void *)(m_pData + m_memOffset);

        pOut->buffer = m_bUseVidMem ? m_bufferVid : m_buffer;
        pOut->offset = m_memOffset;
        pOut->range = size;

        m_memOffset += size;

        return true;
    }

    bool StaticBufferPool::AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void *pInitData, VkDescriptorBufferInfo *pOut)
    {
        void *pData;
        if (AllocBuffer(numbeOfVertices, strideInBytes, &pData, pOut))
        {
            memcpy(pData, pInitData, numbeOfVertices*strideInBytes);
            return true;
        }
        return false;
    }

    void StaticBufferPool::UploadData(VkCommandBuffer cmd_buf)
    {
        VkBufferCopy region;
        region.srcOffset = 0;
        region.dstOffset = 0;
        region.size = m_totalMemSize;

        vkCmdCopyBuffer(cmd_buf, m_buffer, m_bufferVid, 1, &region);
    }

    void StaticBufferPool::FreeUploadHeap()
    {
        if (m_bUseVidMem)
        {
            assert(m_buffer != VK_NULL_HANDLE);
#ifdef USE_VMA
            vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
            vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
            //release upload heap
            vkUnmapMemory(m_pDevice->GetDevice(), m_deviceMemory);
            vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
            m_deviceMemory = VK_NULL_HANDLE;
            vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);

#endif
            m_buffer = VK_NULL_HANDLE;
        }
    }
}




