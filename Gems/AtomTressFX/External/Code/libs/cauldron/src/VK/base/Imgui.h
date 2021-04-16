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

#include "DynamicBufferRing.h"
#include "CommandListRing.h"
#include "UploadHeap.h"
#include "../imgui/imgui.h"
#include "Base/ImGuiHelper.h"

namespace CAULDRON_VK
{
    // This is the vulkan rendering backend for the excellent ImGUI library.

    class ImGUI
    {
    public:
        void OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap *pUploadHeap, DynamicBufferRing *pConstantBufferRing);
        void OnDestroy();

        void UpdatePipeline(VkRenderPass renderPass);
        void Draw(VkCommandBuffer cmd_buf);

    private:
        Device                  *m_pDevice = nullptr;
        DynamicBufferRing       *m_pConstBuf = nullptr;

        VkImage                     m_pTexture2D = VK_NULL_HANDLE;
        VkDeviceMemory              m_deviceMemory;
        VkDescriptorBufferInfo      m_geometry;
        VkPipelineLayout            m_pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool            m_descriptorPool = VK_NULL_HANDLE;
        VkPipeline                  m_pipeline = VK_NULL_HANDLE;
        VkDescriptorSet             m_descriptorSet[128];
        uint32_t                    m_currentDescriptorIndex;
        VkSampler                   m_sampler = VK_NULL_HANDLE;
        VkImageView                 m_pTextureSRV = VK_NULL_HANDLE;
        VkDescriptorSetLayout       m_desc_layout;
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    };
}