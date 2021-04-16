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
#include "DynamicBufferRing.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    VkResult DynamicBufferRing::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char *name)
    {
        VkResult res;
        m_pDevice = pDevice;

        m_memTotalSize = (uint32_t)AlignOffset(memTotalSize, 256);

        m_mem.OnCreate(numberOfBackBuffers, m_memTotalSize);

#ifdef USE_VMA
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = m_memTotalSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocInfo.pUserData = name;

        res = vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_bufferAlloc, nullptr);
        assert(res == VK_SUCCESS);
        res = vmaMapMemory(pDevice->GetAllocator(), m_bufferAlloc, (void **)&m_pData);
        assert(res == VK_SUCCESS);
#else
        // create a buffer that can host uniforms, indices and vertexbuffers
        VkBufferCreateInfo buf_info = {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = NULL;
        buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buf_info.size = m_memTotalSize;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = NULL;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;
        res = vkCreateBuffer(m_pDevice->GetDevice(), &buf_info, NULL, &m_buffer);
        assert(res == VK_SUCCESS);

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), m_buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.memoryTypeIndex = 0;
        alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = 0;

        bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &alloc_info.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        res = vkAllocateMemory(m_pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemory);
        assert(res == VK_SUCCESS);

        res = vkMapMemory(m_pDevice->GetDevice(), m_deviceMemory, 0, mem_reqs.size, 0, (void **)&m_pData);
        assert(res == VK_SUCCESS);

        res = vkBindBufferMemory(m_pDevice->GetDevice(), m_buffer, m_deviceMemory, 0);
        assert(res == VK_SUCCESS);
#endif
        return res;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void DynamicBufferRing::OnDestroy()
    {
#ifdef USE_VMA
        vmaUnmapMemory(m_pDevice->GetAllocator(), m_bufferAlloc);
        vmaDestroyBuffer(m_pDevice->GetAllocator(), m_buffer, m_bufferAlloc);
#else
        vkUnmapMemory(m_pDevice->GetDevice(), m_deviceMemory);
        vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
        vkDestroyBuffer(m_pDevice->GetDevice(), m_buffer, NULL);
#endif
        m_mem.OnDestroy();
    }

    //--------------------------------------------------------------------------------------
    //
    // AllocConstantBuffer
    //
    //--------------------------------------------------------------------------------------
    bool DynamicBufferRing::AllocConstantBuffer(uint32_t size, void **pData, VkDescriptorBufferInfo *pOut)
    {
        size = (uint32_t)AlignOffset(size, 256);

        uint32_t memOffset;
        if (m_mem.Alloc(size, &memOffset) == false)
        {
            assert("Ran out of mem for 'dynamic' buffers, please increase the allocated size");
            return false;
        }

        *pData = (void *)(m_pData + memOffset);

        pOut->buffer = m_buffer;
        pOut->offset = memOffset;
        pOut->range = size;

        return true;
    }

    //--------------------------------------------------------------------------------------
    //
    // AllocVertexBuffer
    //
    //--------------------------------------------------------------------------------------
    bool DynamicBufferRing::AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut)
    {
        return AllocConstantBuffer(numbeOfVertices * strideInBytes, pData, pOut);
    }

    bool DynamicBufferRing::AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut)
    {
        return AllocConstantBuffer(numbeOfIndices * strideInBytes, pData, pOut);
    }

    //--------------------------------------------------------------------------------------
    //
    // OnBeginFrame
    //
    //--------------------------------------------------------------------------------------
    void DynamicBufferRing::OnBeginFrame()
    {
        m_mem.OnBeginFrame();
    }

    void DynamicBufferRing::SetDescriptorSet(int index, uint32_t size, VkDescriptorSet descriptorSet)
    {
        VkDescriptorBufferInfo out = {};
        out.buffer = m_buffer;
        out.offset = 0;
        out.range = size;

        VkWriteDescriptorSet write;
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = descriptorSet;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.pBufferInfo = &out;
        write.dstArrayElement = 0;
        write.dstBinding = index;

        vkUpdateDescriptorSets(m_pDevice->GetDevice(), 1, &write, 0, NULL);
    }
}
