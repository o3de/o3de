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

#include "PostProcCS.h"

namespace CAULDRON_VK
{
    void PostProcCS::OnCreate(
        Device* pDevice,
        const std::string &shaderFilename,
        const std::string &shaderEntryPoint,
        VkDescriptorSetLayout descriptorSetLayout,
        uint32_t dwWidth, uint32_t dwHeight, uint32_t dwDepth,
        DefineList* pUserDefines
    )
    {
        m_pDevice = pDevice;

        VkResult res;

        // Compile shaders
        //
        VkPipelineShaderStageCreateInfo computeShader;
        DefineList defines;
        defines["WIDTH"] = std::to_string(dwWidth);
        defines["HEIGHT"] = std::to_string(dwHeight);
        defines["DEPTH"] = std::to_string(dwDepth);

        if (pUserDefines != NULL)
            defines = *pUserDefines;

        res = VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT, shaderFilename.c_str(), shaderEntryPoint.c_str(), &defines, &computeShader);
        assert(res == VK_SUCCESS);

        // Create pipeline layout
        //

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = NULL;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

        res = vkCreatePipelineLayout(pDevice->GetDevice(), &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
        assert(res == VK_SUCCESS);

        // Create pipeline
        //

        VkComputePipelineCreateInfo pipeline = {};
        pipeline.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline.pNext = NULL;
        pipeline.flags = 0;
        pipeline.layout = m_pipelineLayout;
        pipeline.stage = computeShader;
        pipeline.basePipelineHandle = VK_NULL_HANDLE;
        pipeline.basePipelineIndex = 0;

        res = vkCreateComputePipelines(pDevice->GetDevice(), pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
        assert(res == VK_SUCCESS);
    }

    void PostProcCS::OnDestroy()
    {
        vkDestroyPipeline(m_pDevice->GetDevice(), m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_pipelineLayout, nullptr);
    }

    void PostProcCS::Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo constantBuffer, VkDescriptorSet descSet, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ)
    {
        if (m_pipeline == VK_NULL_HANDLE)
            return;

        // Bind Descriptor sets
        //                
        uint32_t uniformOffsets[1] = { (uint32_t)constantBuffer.offset };
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &descSet, 1, uniformOffsets);

        // Bind Pipeline
        //
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

        // Draw
        //
        vkCmdDispatch(cmd_buf, dispatchX, dispatchY, dispatchZ);
    }
}