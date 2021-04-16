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
#include "Helper.h"

namespace CAULDRON_VK
{
    VkRenderPass SimpleColorWriteRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout)
    {

        // color RT
        VkAttachmentDescription attachments[1];
        attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about the previous contents, this is for a full screen pass with no blending
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = initialLayout;
        attachments[0].finalLayout = finalLayout;
        attachments[0].flags = 0;

        VkAttachmentReference color_reference = { 0, passLayout };

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkSubpassDependency dep = {};
        dep.dependencyFlags = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcSubpass = 0;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dep;

        VkRenderPass renderPass;
        VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &renderPass);
        assert(res == VK_SUCCESS);

        return renderPass;
    }

    VkRenderPass SimpleColorBlendRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout)
    {
        // color RT
        VkAttachmentDescription attachments[1];
        attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = initialLayout;
        attachments[0].finalLayout = finalLayout;
        attachments[0].flags = 0;

        VkAttachmentReference color_reference = { 0, passLayout };

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkSubpassDependency dep = {};
        dep.dependencyFlags = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcSubpass = 0;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dep;

        VkRenderPass renderPass;
        VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &renderPass);
        assert(res == VK_SUCCESS);

        return renderPass;
    }


    void SetViewportAndScissor(VkCommandBuffer cmd_buf, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height)
    {
        VkViewport viewport;
        viewport.x = static_cast<float>(topX);
        viewport.y = static_cast<float>(topY) + static_cast<float>(height);
        viewport.width = static_cast<float>(width);
        viewport.height = -static_cast<float>(height);
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D   scissor;
        scissor.extent.width = (uint32_t)(width);
        scissor.extent.height = (uint32_t)(height);
        scissor.offset.x = topX;
        scissor.offset.y = topY;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
    }

    void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkSampler *pSampler, VkDescriptorSet descriptorSet)
    {
        VkDescriptorImageInfo desc_image;
        desc_image.sampler = (pSampler==NULL)?VK_NULL_HANDLE: *pSampler;
        desc_image.imageView = imageView;
        desc_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write;
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = descriptorSet;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &desc_image;
        write.dstBinding = index;
        write.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }

    VkResult CreateDescriptorSetLayoutVK(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding, VkDescriptorSetLayout *pDescSetLayout, VkDescriptorSet *pDescriptorSet)
    {
        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = NULL;
        descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
        descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

        VkResult res = vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, pDescSetLayout);
        assert(res == VK_SUCCESS);
        return res;
    }
}