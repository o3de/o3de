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
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/ExtDebugMarkers.h"
#include "Base/UploadHeap.h"
#include "Base/Texture.h"
#include "Base/Imgui.h"
#include "Base/Helper.h"

#include "PostProcPS.h"
#include "DownSamplePS.h"

namespace CAULDRON_VK
{
    void DownSamplePS::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pConstantBufferRing,
        StaticBufferPool *pStaticBufferPool,
        VkFormat outFormat
    )
    {
        m_pDevice = pDevice;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_pConstantBufferRing = pConstantBufferRing;
        m_outFormat = outFormat;

        // Create Descriptor Set Layout, the shader needs a uniform dynamic buffer and a texture + sampler
        // The Descriptor Sets will be created and initialized once we know the input to the shader, that happens in OnCreateWindowSizeDependentResources()
        {
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
            layoutBindings[0].binding = 0;
            layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            layoutBindings[0].descriptorCount = 1;
            layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            layoutBindings[0].pImmutableSamplers = NULL;

            layoutBindings[1].binding = 1;
            layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutBindings[1].descriptorCount = 1;
            layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            layoutBindings[1].pImmutableSamplers = NULL;

            VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
            descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptor_layout.pNext = NULL;
            descriptor_layout.bindingCount = (uint32_t)layoutBindings.size();
            descriptor_layout.pBindings = layoutBindings.data();

            VkResult res = vkCreateDescriptorSetLayout(pDevice->GetDevice(), &descriptor_layout, NULL, &m_descriptorSetLayout);
            assert(res == VK_SUCCESS);
        }

        // In Render pass
        //
        m_in = SimpleColorWriteRenderPass(pDevice->GetDevice(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // The sampler we want to use for downsampling, all linear
        //
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_sampler);
            assert(res == VK_SUCCESS);
        }

        // Use helper class to create the fullscreen pass
        //
        m_downscale.OnCreate(pDevice, m_in, "DownSamplePS.glsl", pStaticBufferPool, pConstantBufferRing, m_descriptorSetLayout);

        // Allocate descriptors for the mip chain
        //
        for (int i = 0; i < DOWNSAMPLEPS_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_mip[i].descriptorSet);
        }
    }

    void DownSamplePS::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture *pInput, int mipCount)
    {
        m_Width = Width;
        m_Height = Height;
        m_mipCount = mipCount;

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = m_outFormat;
        image_info.extent.width = m_Width >> 1;
        image_info.extent.height = m_Height >> 1;
        image_info.extent.depth = 1;
        image_info.mipLevels = mipCount;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.usage = (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT); //TODO    
        image_info.flags = 0;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        m_result.Init(m_pDevice, &image_info, "DownsampleMip");

        // Create views for the mip chain
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            // source -----------
            //
            if (i == 0)
            {
                pInput->CreateSRV(&m_mip[i].m_SRV, 0);
            }
            else
            {
                m_result.CreateSRV(&m_mip[i].m_SRV, i - 1);
            }

            // Create and initialize the Descriptor Sets (all of them use the same Descriptor Layout)        
            m_pConstantBufferRing->SetDescriptorSet(0, sizeof(DownSamplePS::cbDownscale), m_mip[i].descriptorSet);
            SetDescriptorSet(m_pDevice->GetDevice(), 1, m_mip[i].m_SRV, &m_sampler, m_mip[i].descriptorSet);

            // destination -----------
            //
            m_result.CreateRTV(&m_mip[i].RTV, i);

            // Create framebuffer 
            {
                VkImageView attachments[1] = { m_mip[i].RTV };

                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext = NULL;
                fb_info.renderPass = m_in;
                fb_info.attachmentCount = 1;
                fb_info.pAttachments = attachments;
                fb_info.width = m_Width >> (i + 1);
                fb_info.height = m_Height >> (i + 1);
                fb_info.layers = 1;
                VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_mip[i].frameBuffer);
                assert(res == VK_SUCCESS);
            }
        }
    }

    void DownSamplePS::OnDestroyWindowSizeDependentResources()
    {
        for (int i = 0; i < m_mipCount; i++)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_mip[i].m_SRV, NULL);
            vkDestroyImageView(m_pDevice->GetDevice(), m_mip[i].RTV, NULL);
            vkDestroyFramebuffer(m_pDevice->GetDevice(), m_mip[i].frameBuffer, NULL);
        }

        m_result.OnDestroy();
    }

    void DownSamplePS::OnDestroy()
    {
        for (int i = 0; i < DOWNSAMPLEPS_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->FreeDescriptor(m_mip[i].descriptorSet);
        }

        m_downscale.OnDestroy();
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, NULL);
        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);

        vkDestroyRenderPass(m_pDevice->GetDevice(), m_in, NULL);
    }

    void DownSamplePS::Draw(VkCommandBuffer cmd_buf)
    {
        SetPerfMarkerBegin(cmd_buf, "Downsample");

        // downsample
        //
        for (int i = 0; i < m_mipCount; i++)
        {

            VkRenderPassBeginInfo rp_begin = {};
            rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rp_begin.pNext = NULL;
            rp_begin.renderPass = m_in;
            rp_begin.framebuffer = m_mip[i].frameBuffer;
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent.width = m_Width >> (i + 1);
            rp_begin.renderArea.extent.height = m_Height >> (i + 1);
            rp_begin.clearValueCount = 0;
            rp_begin.pClearValues = NULL;
            vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
            SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> (i + 1), m_Height >> (i + 1));

            cbDownscale *data;
            VkDescriptorBufferInfo constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(cbDownscale), (void **)&data, &constantBuffer);
            data->invWidth = 1.0f / (float)(m_Width >> i);
            data->invHeight = 1.0f / (float)(m_Height >> i);
            data->mipLevel = i;

            m_downscale.Draw(cmd_buf, constantBuffer, m_mip[i].descriptorSet);

            vkCmdEndRenderPass(cmd_buf);
        }

        SetPerfMarkerEnd(cmd_buf);
    }

    void DownSamplePS::Gui()
    {
        bool opened = true;
        ImGui::Begin("DownSamplePS", &opened);

        for (int i = 0; i < m_mipCount; i++)
        {
            ImGui::Image((ImTextureID)m_mip[i].m_SRV, ImVec2(320 / 2, 180 / 2));
        }

        ImGui::End();
    }
}
