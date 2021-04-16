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
#include "Base/Helper.h"
#include "PostProcPS.h"
#include "BlurPS.h"

namespace CAULDRON_VK
{
    void BlurPS::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pConstantBufferRing,
        StaticBufferPool *pStaticBufferPool,
        VkFormat format
    )
    {
        m_pDevice = pDevice;
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_pConstantBufferRing = pConstantBufferRing;

        m_outFormat = format;

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

        // Create a Render pass that will discard the contents of the render target.
        //
        m_in = SimpleColorWriteRenderPass(pDevice->GetDevice(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // The sampler we want to use for downsampling, all linear
        //
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
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
        m_directionalBlur.OnCreate(pDevice, m_in, "blur.glsl", pStaticBufferPool, pConstantBufferRing, m_descriptorSetLayout);

        // Allocate descriptors for the mip chain
        //
        for (int i = 0; i < BLURPS_MAX_MIP_LEVELS; i++)
        {
            // Horizontal pass
            //
            m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_horizontalMip[i].m_descriptorSet);

            // Vertical pass
            //
            m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_verticalMip[i].m_descriptorSet);
        }

    }

    void BlurPS::OnCreateWindowSizeDependentResources(Device* pDevice, uint32_t Width, uint32_t Height, Texture *pInput, int mipCount)
    {
        m_Width = Width;
        m_Height = Height;
        m_mipCount = mipCount;

        // Create a temporary texture to hold the horizontal pass (only now we know the size of the render target we want to downsample, hence we create the temporary render target here)
        //
        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = m_outFormat;
        image_info.extent.width = m_Width;
        image_info.extent.height = m_Height;
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
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster
        m_tempBlur.Init(m_pDevice, &image_info, "BlurHorizontal");

        // Create framebuffers and views for the mip chain
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            // Horizontal pass, from pInput to m_tempBlur
            //
            {
                pInput->CreateSRV(&m_horizontalMip[i].m_SRV, i);     // source (pInput)
                m_tempBlur.CreateRTV(&m_horizontalMip[i].m_RTV, i);  // target (m_tempBlur)

                // Create framebuffer for target
                //
                {
                    VkImageView attachments[1] = { m_horizontalMip[i].m_RTV };

                    VkFramebufferCreateInfo fb_info = {};
                    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    fb_info.pNext = NULL;
                    fb_info.renderPass = m_in;
                    fb_info.attachmentCount = 1;
                    fb_info.pAttachments = attachments;
                    fb_info.width = Width >> i;
                    fb_info.height = Height >> i;
                    fb_info.layers = 1;
                    VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_horizontalMip[i].m_frameBuffer);
                    assert(res == VK_SUCCESS);
                }

                // Create Descriptor sets (all of them use the same Descriptor Layout)            
                m_pConstantBufferRing->SetDescriptorSet(0, sizeof(BlurPS::cbBlur), m_horizontalMip[i].m_descriptorSet);
                SetDescriptorSet(m_pDevice->GetDevice(), 1, m_horizontalMip[i].m_SRV, &m_sampler, m_horizontalMip[i].m_descriptorSet);
            }

            // Vertical pass, from m_tempBlur back to pInput
            //
            {
                m_tempBlur.CreateSRV(&m_verticalMip[i].m_SRV, i);   // source (pInput)(m_tempBlur)
                pInput->CreateRTV(&m_verticalMip[i].m_RTV, i);      // target (pInput)

                {
                    VkImageView attachments[1] = { m_verticalMip[i].m_RTV };

                    VkFramebufferCreateInfo fb_info = {};
                    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    fb_info.pNext = NULL;
                    fb_info.renderPass = m_in;
                    fb_info.attachmentCount = 1;
                    fb_info.pAttachments = attachments;
                    fb_info.width = Width >> i;
                    fb_info.height = Height >> i;
                    fb_info.layers = 1;
                    VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_verticalMip[i].m_frameBuffer);
                    assert(res == VK_SUCCESS);
                }

                // create and update descriptor
                m_pConstantBufferRing->SetDescriptorSet(0, sizeof(BlurPS::cbBlur), m_verticalMip[i].m_descriptorSet);
                SetDescriptorSet(m_pDevice->GetDevice(), 1, m_verticalMip[i].m_SRV, &m_sampler, m_verticalMip[i].m_descriptorSet);
            }
        }
    }

    void BlurPS::OnDestroyWindowSizeDependentResources()
    {
        // destroy views and framebuffers of the vertical and horizontal passes
        //
        for (int i = 0; i < m_mipCount; i++)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_horizontalMip[i].m_SRV, NULL);
            vkDestroyImageView(m_pDevice->GetDevice(), m_horizontalMip[i].m_RTV, NULL);
            vkDestroyFramebuffer(m_pDevice->GetDevice(), m_horizontalMip[i].m_frameBuffer, NULL);

            vkDestroyImageView(m_pDevice->GetDevice(), m_verticalMip[i].m_SRV, NULL);
            vkDestroyImageView(m_pDevice->GetDevice(), m_verticalMip[i].m_RTV, NULL);
            vkDestroyFramebuffer(m_pDevice->GetDevice(), m_verticalMip[i].m_frameBuffer, NULL);
        }

        // Destroy temporary render target used to hold the horizontal pass
        //
        m_tempBlur.OnDestroy();
    }

    void BlurPS::OnDestroy()
    {
        // destroy views
        for (int i = 0; i < BLURPS_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->FreeDescriptor(m_horizontalMip[i].m_descriptorSet);
            m_pResourceViewHeaps->FreeDescriptor(m_verticalMip[i].m_descriptorSet);
        }

        m_directionalBlur.OnDestroy();
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, NULL);
        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);
        vkDestroyRenderPass(m_pDevice->GetDevice(), m_in, NULL);
    }

    void BlurPS::Draw(VkCommandBuffer cmd_buf, int mipLevel)
    {
        SetPerfMarkerBegin(cmd_buf, "blur");

        SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> mipLevel, m_Height >> mipLevel);

        // horizontal pass
        //
        {
            VkRenderPassBeginInfo rp_begin = {};
            rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rp_begin.pNext = NULL;
            rp_begin.renderPass = m_in;
            rp_begin.framebuffer = m_horizontalMip[mipLevel].m_frameBuffer;
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent.width = m_Width >> mipLevel;
            rp_begin.renderArea.extent.height = m_Height >> mipLevel;
            rp_begin.clearValueCount = 0;
            rp_begin.pClearValues = NULL;
            vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

            BlurPS::cbBlur *data;
            VkDescriptorBufferInfo constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(BlurPS::cbBlur), (void **)&data, &constantBuffer);
            data->dirX = 1.0f / (float)(m_Width >> mipLevel);
            data->dirY = 0.0f / (float)(m_Height >> mipLevel);
            data->mipLevel = mipLevel;
            m_directionalBlur.Draw(cmd_buf, constantBuffer, m_horizontalMip[mipLevel].m_descriptorSet);

            vkCmdEndRenderPass(cmd_buf);
        }

        // vertical pass
        //
        {
            VkRenderPassBeginInfo rp_begin = {};
            rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rp_begin.pNext = NULL;
            rp_begin.renderPass = m_in;
            rp_begin.framebuffer = m_verticalMip[mipLevel].m_frameBuffer;
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent.width = m_Width >> mipLevel;
            rp_begin.renderArea.extent.height = m_Height >> mipLevel;
            rp_begin.clearValueCount = 0;
            rp_begin.pClearValues = NULL;
            vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

            BlurPS::cbBlur *data;
            VkDescriptorBufferInfo constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(BlurPS::cbBlur), (void **)&data, &constantBuffer);
            data->dirX = 0.0f / (float)(m_Width >> mipLevel);
            data->dirY = 1.0f / (float)(m_Height >> mipLevel);
            data->mipLevel = mipLevel;
            m_directionalBlur.Draw(cmd_buf, constantBuffer, m_verticalMip[mipLevel].m_descriptorSet);

            vkCmdEndRenderPass(cmd_buf);
        }
        
        SetPerfMarkerEnd(cmd_buf);
    }

    void BlurPS::Draw(VkCommandBuffer cmd_buf)
    {
        for (int i = 0; i < m_mipCount; i++)
        {
            Draw(cmd_buf, i);
        }
    }
}