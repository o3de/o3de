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
#include "ResourceViewHeaps.h"
#include "Misc/misc.h"

namespace CAULDRON_VK
{
    void ResourceViewHeaps::OnCreate(Device *pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount, uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount)
    {
        m_pDevice = pDevice;
        m_allocatedDescriptorCount = 0;

        std::vector<VkDescriptorPoolSize> type_count =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, cbvDescriptorCount },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, srvDescriptorCount },
            { VK_DESCRIPTOR_TYPE_SAMPLER, samplerDescriptorCount },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uavDescriptorCount }
        };

        VkDescriptorPoolCreateInfo descriptor_pool = {};
        descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool.pNext = NULL;
        descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptor_pool.maxSets = 6000;
        descriptor_pool.poolSizeCount = (uint32_t)type_count.size();
        descriptor_pool.pPoolSizes = type_count.data();

        VkResult res = vkCreateDescriptorPool(pDevice->GetDevice(), &descriptor_pool, NULL, &m_descriptorPool);
        assert(res == VK_SUCCESS);
    }

    void ResourceViewHeaps::OnDestroy()
    {
        vkDestroyDescriptorPool(m_pDevice->GetDevice(), m_descriptorPool, NULL);
    }

    bool ResourceViewHeaps::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding, VkDescriptorSetLayout *pDescSetLayout)
    {
        // Next take layout bindings and use them to create a descriptor set layout

        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = NULL;
        descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
        descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

        VkResult res = vkCreateDescriptorSetLayout(m_pDevice->GetDevice(), &descriptor_layout, NULL, pDescSetLayout);
        assert(res == VK_SUCCESS);
        return (res == VK_SUCCESS);
    }
    bool ResourceViewHeaps::CreateDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding, VkDescriptorSetLayout *pDescSetLayout, VkDescriptorSet *pDescriptorSet)
    {
        // Next take layout bindings and use them to create a descriptor set layout

        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = NULL;
        descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
        descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

        VkResult res = vkCreateDescriptorSetLayout(m_pDevice->GetDevice(), &descriptor_layout, NULL, pDescSetLayout);
        assert(res == VK_SUCCESS);

        return AllocDescriptor(*pDescSetLayout, pDescriptorSet);
    }

    bool ResourceViewHeaps::AllocDescriptor(VkDescriptorSetLayout descLayout, VkDescriptorSet *pDescriptorSet)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.descriptorPool = m_descriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &descLayout;

        VkResult res = vkAllocateDescriptorSets(m_pDevice->GetDevice(), &alloc_info, pDescriptorSet);
        assert(res == VK_SUCCESS);

        m_allocatedDescriptorCount++;

        return res == VK_SUCCESS;
    }

    void ResourceViewHeaps::FreeDescriptor(VkDescriptorSet descriptorSet)
    {
        m_allocatedDescriptorCount--;
        vkFreeDescriptorSets(m_pDevice->GetDevice(), m_descriptorPool, 1, &descriptorSet);
    }

    bool ResourceViewHeaps::AllocDescriptor(int size, const VkSampler *pSamplers, VkDescriptorSetLayout *pDescSetLayout, VkDescriptorSet *pDescriptorSet)
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
        for (int i = 0; i < size; i++)
        {
            layoutBindings[i].binding = i;
            layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutBindings[i].descriptorCount = 1;
            layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            layoutBindings[i].pImmutableSamplers = (pSamplers!=NULL)? &pSamplers[i]:NULL;
        }

        return CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
    }
}


