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
#include "Base/Imgui.h"
#include "Base/Helper.h"
#include "Base/Texture.h"

#include "PostProcPS.h"

#include "Bloom.h"

namespace CAULDRON_VK
{
    void Bloom::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pConstantBufferRing,
        StaticBufferPool *pStaticBufferPool,
        VkFormat outFormat
    )
    {
        m_pDevice = pDevice;
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_pConstantBufferRing = pConstantBufferRing;
        m_outFormat = outFormat;

        m_blur.OnCreate(pDevice, m_pResourceViewHeaps, m_pConstantBufferRing, pStaticBufferPool, m_outFormat);

        // Create Descriptor Set Layout (for each mip level we will create later on the individual Descriptor Sets)
        //
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

        // Create a Render pass that accounts for blending
        //
        m_blendPass = SimpleColorBlendRenderPass(pDevice->GetDevice(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        //blending add
        {
            VkPipelineColorBlendAttachmentState att_state[1];
            att_state[0].colorWriteMask = 0xf;
            att_state[0].blendEnable = VK_TRUE;
            att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
            att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
            att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
            att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

            VkPipelineColorBlendStateCreateInfo cb;
            cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cb.flags = 0;
            cb.pNext = NULL;
            cb.attachmentCount = 1;
            cb.pAttachments = att_state;
            cb.logicOpEnable = VK_FALSE;
            cb.logicOp = VK_LOGIC_OP_NO_OP;
            cb.blendConstants[0] = 1.0f;
            cb.blendConstants[1] = 1.0f;
            cb.blendConstants[2] = 1.0f;
            cb.blendConstants[3] = 1.0f;

            m_blendAdd.OnCreate(pDevice, m_blendPass, "blend.glsl", pStaticBufferPool, pConstantBufferRing, m_descriptorSetLayout, &cb);
        }

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

        // Allocate descriptors for the mip chain
        //
        for (int i = 0; i < BLOOM_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_mip[i].m_descriptorSet);
        }

        // Allocate descriptors for the output pass
        //
        m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_output.m_descriptorSet);

        m_doBlur = true;
        m_doUpscale = true;
    }

    void Bloom::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture *pInput, int mipCount, Texture *pOutput)
    {
        m_Width = Width;
        m_Height = Height;
        m_mipCount = mipCount;

        m_blur.OnCreateWindowSizeDependentResources(m_pDevice, Width, Height, pInput, mipCount);

        for (int i = 0; i < m_mipCount; i++)
        {
            pInput->CreateSRV(&m_mip[i].m_SRV, i);     // source (pInput)
            pInput->CreateRTV(&m_mip[i].m_RTV, i);     // target (pInput)

            // Create framebuffer for target
            //
            {
                VkImageView attachments[1] = { m_mip[i].m_RTV };

                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext = NULL;
                fb_info.renderPass = m_blendPass;
                fb_info.attachmentCount = 1;
                fb_info.pAttachments = attachments;
                fb_info.width = Width >> i;
                fb_info.height = Height >> i;
                fb_info.layers = 1;
                VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_mip[i].m_frameBuffer);
                assert(res == VK_SUCCESS);
            }

            // Set descriptors        
            m_pConstantBufferRing->SetDescriptorSet(0, sizeof(Bloom::cbBlend), m_mip[i].m_descriptorSet);
            SetDescriptorSet(m_pDevice->GetDevice(), 1, m_mip[i].m_SRV, &m_sampler, m_mip[i].m_descriptorSet);
        }

        {
            // output pass
            pOutput->CreateRTV(&m_output.m_RTV, 0);

            // Create framebuffer for target
            //
            {
                VkImageView attachments[1] = { m_output.m_RTV };

                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext = NULL;
                fb_info.renderPass = m_blendPass;
                fb_info.attachmentCount = 1;
                fb_info.pAttachments = attachments;
                fb_info.width = Width * 2;
                fb_info.height = Height * 2;
                fb_info.layers = 1;
                VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_output.m_frameBuffer);
                assert(res == VK_SUCCESS);
            }

            // Set descriptors        
            m_pConstantBufferRing->SetDescriptorSet(0, sizeof(Bloom::cbBlend), m_output.m_descriptorSet);
            SetDescriptorSet(m_pDevice->GetDevice(), 1, m_mip[1].m_SRV, &m_sampler, m_output.m_descriptorSet);
        }

        // set weights of each mip level
        m_mip[0].m_weight = 1.0f - 0.08f;
        m_mip[1].m_weight = 0.25f;
        m_mip[2].m_weight = 0.75f;
        m_mip[3].m_weight = 1.5f;
        m_mip[4].m_weight = 2.5f;
        m_mip[5].m_weight = 3.0f;

        // normalize weights
        float n = 0;
        for (uint32_t i = 1; i < 6; i++)
            n += m_mip[i].m_weight;
        
        for (uint32_t i = 1; i < 6; i++)
            m_mip[i].m_weight /= n;
    }

    void Bloom::OnDestroyWindowSizeDependentResources()
    {
        m_blur.OnDestroyWindowSizeDependentResources();

        for (int i = 0; i < m_mipCount; i++)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_mip[i].m_SRV, NULL);
            vkDestroyImageView(m_pDevice->GetDevice(), m_mip[i].m_RTV, NULL);
            vkDestroyFramebuffer(m_pDevice->GetDevice(), m_mip[i].m_frameBuffer, NULL);
        }

        vkDestroyImageView(m_pDevice->GetDevice(), m_output.m_RTV, NULL);
        vkDestroyFramebuffer(m_pDevice->GetDevice(), m_output.m_frameBuffer, NULL);
    }

    void Bloom::OnDestroy()
    {
        for (int i = 0; i < BLOOM_MAX_MIP_LEVELS; i++)
        {
            m_pResourceViewHeaps->FreeDescriptor(m_mip[i].m_descriptorSet);
        }

        m_pResourceViewHeaps->FreeDescriptor(m_output.m_descriptorSet);

        m_blur.OnDestroy();

        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, NULL);
        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);

        m_blendAdd.OnDestroy();

        vkDestroyRenderPass(m_pDevice->GetDevice(), m_blendPass, NULL);
    }

    void Bloom::Draw(VkCommandBuffer cmd_buf)
    {
        SetPerfMarkerBegin(cmd_buf, "Bloom");

        //float weights[6] = { 0.25, 0.75, 1.5, 2, 2.5, 3.0 };

        // given a RT, and its mip chain m0, m1, m2, m3, m4 
        // 
        // m4 = blur(m5)
        // m4 = blur(m4) + w5 *m5
        // m3 = blur(m3) + w4 *m4
        // m2 = blur(m2) + w3 *m3
        // m1 = blur(m1) + w2 *m2
        // m0 = blur(m0) + w1 *m1
        // RT = 0.92 * RT + 0.08 * m0

        // blend and upscale
        for (int i = m_mipCount - 1; i >= 0; i--)
        {
            // blur mip level
            //
            if (m_doBlur)
            {                
                m_blur.Draw(cmd_buf, i);
            }

            // blend with mip above   
            SetPerfMarkerBegin(cmd_buf, "blend above");

            Bloom::cbBlend *data;
            VkDescriptorBufferInfo constantBuffer;
            m_pConstantBufferRing->AllocConstantBuffer(sizeof(Bloom::cbBlend), (void **)&data, &constantBuffer);

            if (i != 0)
            {
                VkRenderPassBeginInfo rp_begin = {};
                rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rp_begin.pNext = NULL;
                rp_begin.renderPass = m_blendPass;
                rp_begin.framebuffer = m_mip[i - 1].m_frameBuffer;
                rp_begin.renderArea.offset.x = 0;
                rp_begin.renderArea.offset.y = 0;
                rp_begin.renderArea.extent.width = m_Width >> (i - 1);
                rp_begin.renderArea.extent.height = m_Height >> (i - 1);
                rp_begin.clearValueCount = 0;
                rp_begin.pClearValues = NULL;
                vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

                SetViewportAndScissor(cmd_buf, 0, 0, m_Width >> (i - 1), m_Height >> (i - 1));

                float blendConstants[4] = { m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight };
                vkCmdSetBlendConstants(cmd_buf, blendConstants);

                data->weight = 1.0f;
            }
            else
            {
                //composite
                VkRenderPassBeginInfo rp_begin = {};
                rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rp_begin.pNext = NULL;
                rp_begin.renderPass = m_blendPass;
                rp_begin.framebuffer = m_output.m_frameBuffer;
                rp_begin.renderArea.offset.x = 0;
                rp_begin.renderArea.offset.y = 0;
                rp_begin.renderArea.extent.width = m_Width * 2;
                rp_begin.renderArea.extent.height = m_Height * 2;
                rp_begin.clearValueCount = 0;
                rp_begin.pClearValues = NULL;
                vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

                SetViewportAndScissor(cmd_buf, 0, 0, m_Width * 2, m_Height * 2);

                float blendConstants[4] = { m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight, m_mip[i].m_weight };
                vkCmdSetBlendConstants(cmd_buf, blendConstants);

                data->weight = 1.0f - m_mip[i].m_weight;
            }

            if (m_doUpscale)
                m_blendAdd.Draw(cmd_buf, constantBuffer, m_mip[i].m_descriptorSet);

            vkCmdEndRenderPass(cmd_buf);
            SetPerfMarkerEnd(cmd_buf);
        }

        SetPerfMarkerEnd(cmd_buf);
    }

    void Bloom::Gui()
    {
        bool opened = true;
        if (ImGui::Begin("Bloom Controls", &opened))
        {
            ImGui::Checkbox("Blur Bloom Stages", &m_doBlur);
            ImGui::Checkbox("Upscaling", &m_doUpscale);

            ImGui::SliderFloat("weight 0", &m_mip[0].m_weight, 0.0f, 1.0f);

            for (int i = 1; i < m_mipCount; i++)
            {
                char buf[32];
                sprintf_s<32>(buf, "weight %i", i);
                ImGui::SliderFloat(buf, &m_mip[i].m_weight, 0.0f, 4.0f);
            }
        }
        ImGui::End();
    }
}
