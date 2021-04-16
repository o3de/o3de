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
#include "Base/Device.h"
#include "Base/ShaderCompilerHelper.h"
#include "CheckerboardFloor.h"

namespace CAULDRON_VK
{
    void CheckerBoardFloor::OnCreate(
        Device* pDevice,
        VkRenderPass renderPass,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        VkSampleCountFlagBits sampleDescCount)
    {
        VkResult res;

        m_pDevice = pDevice;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pResourceViewHeaps = pResourceViewHeaps;

        // set indices
        {
            short indices[] =
            {
                0,1,2,
                0,2,3
            };
            m_NumIndices = _countof(indices);
            m_indexType = VK_INDEX_TYPE_UINT16;

            m_pStaticBufferPool->AllocBuffer(m_NumIndices, sizeof(short), indices, &m_IBV);
        }

        // set vertices
        {
            float vertices[] =
            {
                -1,  0, -1,      0, 0,
                 1,  0, -1,      1, 0,
                 1,  0,  1,      1, 1,
                -1,  0,  1,      0, 1,
            };

            m_pStaticBufferPool->AllocBuffer(4, 5 * sizeof(float), vertices, &m_VBV);
        }

        // the vertex shader
        static const char* vertexShader =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform _cbPerObject\n"
            "{\n"
            "    mat4        u_mWorldViewProj;\n"
            "    vec4        u_Color;\n"
            "} cbPerObject;\n"
            "layout(location = 0) in vec3 position; \n"
            "layout(location = 1) in vec2 inTexCoord; \n"
            "layout (location = 0) out vec4 outColor;\n"
            "layout (location = 1) out vec2 outTexCoord;\n"
            "void main() {\n"
            "   float size = 1000.0;\n"
            "   outColor = cbPerObject.u_Color;\n"
            "   outTexCoord = inTexCoord * size;\n"
            "   gl_Position = cbPerObject.u_mWorldViewProj * vec4(position.xyz*size,1.0);\n"
            "}\n";

        // the pixel shader
        static const char* pixelShader =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 inColor;\n"
            "layout(location = 1) in vec2 inTexCoord; \n"
            "layout (location = 0) out vec4 outColor;\n"
            "\n"
            "// http://iquilezles.org/www/articles/checkerfiltering/checkerfiltering.htm\n"
            "float checkersGradBox(in vec2 p)\n"
            "{\n"
            "   // filter kernel\n"
            "   vec2 w = fwidth(p) + 0.001;\n"
            "   // analytical integral (box filter)\n"
            "   vec2 i = 2.0*(abs(fract((p - 0.5*w)*0.5) - 0.5) - abs(fract((p + 0.5*w)*0.5) - 0.5)) / w;\n"
            "   // xor pattern\n"
            "   return 0.5 - 0.5*i.x*i.y;\n"
            "}\n"
            "\n"
            "void main() {\n"
            "   float f = checkersGradBox(inTexCoord);\n"
            "   float k = 0.3 + f*0.1;\n"
            "   outColor = vec4(inColor.rgb*k,1.0);\n"
            "}";

        /////////////////////////////////////////////
        // Compile and create shaders

        DefineList attributeDefines;

        VkPipelineShaderStageCreateInfo m_vertexShader;
        res = VKCompileFromString(pDevice->GetDevice(), SST_GLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", &attributeDefines, &m_vertexShader);
        assert(res == VK_SUCCESS);

        VkPipelineShaderStageCreateInfo m_fragmentShader;
        res = VKCompileFromString(pDevice->GetDevice(), SST_GLSL, VK_SHADER_STAGE_FRAGMENT_BIT, pixelShader, "main", &attributeDefines, &m_fragmentShader);
        assert(res == VK_SUCCESS);

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { m_vertexShader, m_fragmentShader };

        /////////////////////////////////////////////
        // Create descriptor set layout

        /////////////////////////////////////////////
        // Create descriptor set 

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[0].pImmutableSamplers = NULL;

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, &m_descriptorSetLayout, &m_descriptorSet);
        m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(per_object), m_descriptorSet);

        /////////////////////////////////////////////
        // Create the pipeline layout using the descriptoset

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = NULL;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
        pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)1;
        pPipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

        res = vkCreatePipelineLayout(pDevice->GetDevice(), &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
        assert(res == VK_SUCCESS);

        /////////////////////////////////////////////
        // Create pipeline

        // Create the input attribute description / input layout(in DX12 jargon)
        //
        VkVertexInputBindingDescription vi_binding = {};
        vi_binding.binding = 0;
        vi_binding.stride = sizeof(float) * 5;
        vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vi_attrs[] =
        {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3 },
        };

        VkPipelineVertexInputStateCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = NULL;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &vi_binding;
        vi.vertexAttributeDescriptionCount = _countof(vi_attrs);
        vi.pVertexAttributeDescriptions = vi_attrs;

        // input assembly state
        //
        VkPipelineInputAssemblyStateCreateInfo ia;
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.pNext = NULL;
        ia.flags = 0;
        ia.primitiveRestartEnable = VK_FALSE;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // rasterizer state
        //
        VkPipelineRasterizationStateCreateInfo rs;
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.pNext = NULL;
        rs.flags = 0;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.depthBiasEnable = VK_FALSE;
        rs.depthBiasConstantFactor = 0;
        rs.depthBiasClamp = 0;
        rs.depthBiasSlopeFactor = 0;
        rs.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState att_state[1];
        att_state[0].colorWriteMask = 0xf;
        att_state[0].blendEnable = VK_TRUE;
        att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        // Color blend state
        //
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

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pNext = NULL;
        dynamicState.pDynamicStates = dynamicStateEnables.data();
        dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

        // view port state
        //
        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.pNext = NULL;
        vp.flags = 0;
        vp.viewportCount = 1;
        vp.scissorCount = 1;
        vp.pScissors = NULL;
        vp.pViewports = NULL;

        // depth stencil state
        //
        VkPipelineDepthStencilStateCreateInfo ds;
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext = NULL;
        ds.flags = 0;
        ds.depthTestEnable = true;
        ds.depthWriteEnable = true;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.stencilTestEnable = VK_FALSE;
        ds.back.failOp = VK_STENCIL_OP_KEEP;
        ds.back.passOp = VK_STENCIL_OP_KEEP;
        ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
        ds.back.compareMask = 0;
        ds.back.reference = 0;
        ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.back.writeMask = 0;
        ds.minDepthBounds = 0;
        ds.maxDepthBounds = 0;
        ds.stencilTestEnable = VK_FALSE;
        ds.front = ds.back;

        // multi sample state
        //
        VkPipelineMultisampleStateCreateInfo ms;
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.pNext = NULL;
        ms.flags = 0;
        ms.pSampleMask = NULL;
        ms.rasterizationSamples = sampleDescCount;
        ms.sampleShadingEnable = VK_FALSE;
        ms.alphaToCoverageEnable = VK_FALSE;
        ms.alphaToOneEnable = VK_FALSE;
        ms.minSampleShading = 0.0;

        // create pipeline 

        VkGraphicsPipelineCreateInfo pipeline = {};
        pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline.pNext = NULL;
        pipeline.layout = m_pipelineLayout;
        pipeline.basePipelineHandle = VK_NULL_HANDLE;
        pipeline.basePipelineIndex = 0;
        pipeline.flags = 0;
        pipeline.pVertexInputState = &vi;
        pipeline.pInputAssemblyState = &ia;
        pipeline.pRasterizationState = &rs;
        pipeline.pColorBlendState = &cb;
        pipeline.pTessellationState = NULL;
        pipeline.pMultisampleState = &ms;
        pipeline.pDynamicState = &dynamicState;
        pipeline.pViewportState = &vp;
        pipeline.pDepthStencilState = &ds;
        pipeline.pStages = shaderStages.data();
        pipeline.stageCount = (uint32_t)shaderStages.size();
        pipeline.renderPass = renderPass;
        pipeline.subpass = 0;

        res = vkCreateGraphicsPipelines(pDevice->GetDevice(), pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
        assert(res == VK_SUCCESS);

    }

    void CheckerBoardFloor::OnDestroy()
    {
        vkDestroyPipeline(m_pDevice->GetDevice(), m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, NULL);
        m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);
    }

    void CheckerBoardFloor::Draw(VkCommandBuffer cmd_buf, XMMATRIX worldMatrix, XMVECTOR vColor)
    {
        if (m_pipeline == VK_NULL_HANDLE)
            return;

        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_VBV.buffer, &m_VBV.offset);
        vkCmdBindIndexBuffer(cmd_buf, m_IBV.buffer, m_IBV.offset, m_indexType);

        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        VkDescriptorSet descritorSets[1] = { m_descriptorSet };

        // Set per Object constants
        //
        per_object *cbPerObject;
        VkDescriptorBufferInfo perObjectDesc;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), (void **)&cbPerObject, &perObjectDesc);
        cbPerObject->m_mWorldViewProj = worldMatrix;
        cbPerObject->m_vColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);

        uint32_t uniformOffsets[1] = { (uint32_t)perObjectDesc.offset };
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descritorSets, 1, uniformOffsets);

        vkCmdDrawIndexed(cmd_buf, m_NumIndices, 1, 0, 0, 0);
    }
}
