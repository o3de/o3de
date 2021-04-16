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
#pragma once

#include "vulkan/vulkan.h"

namespace CAULDRON_VK
{
    // Sets the viewport and the scissor to a fixed height and width
    //
    void SetViewportAndScissor(VkCommandBuffer cmd_buf, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height);

    // Creates a Render pass that will discard the contents of the render target.
    //
    VkRenderPass SimpleColorWriteRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout);

    // Creates a Render pass that will use the contents of the render target for blending.
    //
    VkRenderPass SimpleColorBlendRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout);

    // Sets the i-th Descriptor Set entry to use a given image view + sampler. The sampler can be null is a static one is being used.
    //
    void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkSampler *pSampler, VkDescriptorSet descriptorSet);
}