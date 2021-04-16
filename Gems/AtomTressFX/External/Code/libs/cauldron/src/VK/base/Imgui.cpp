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
#include "ShaderCompilerHelper.h"
#include "Base/ExtDebugMarkers.h"
#include "Imgui.h"

namespace CAULDRON_VK
{
    // Data
    static HWND                     g_hWnd = 0;

    struct VERTEX_CONSTANT_BUFFER
    {
        float        mvp[4][4];
    };

    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap *pUploadHeap, DynamicBufferRing *pConstantBufferRing)
    {
        m_pConstBuf = pConstantBufferRing;
        m_pDevice = pDevice;
        m_currentDescriptorIndex = 0;

        VkResult res;

        // Get UI texture 
        //
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        size_t upload_size = width * height * 4 * sizeof(char);

        // Create the texture object
        //
        {
            VkImageCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = VK_FORMAT_R8G8B8A8_UNORM;
            info.extent.width = width;
            info.extent.height = height;
            info.extent.depth = 1;
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            res = vkCreateImage(pDevice->GetDevice(), &info, NULL, &m_pTexture2D);
            assert(res == VK_SUCCESS);

            VkMemoryRequirements mem_reqs;
            vkGetImageMemoryRequirements(pDevice->GetDevice(), m_pTexture2D, &mem_reqs);

            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_reqs.size;
            alloc_info.memoryTypeIndex = 0;

            bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &alloc_info.memoryTypeIndex);
            assert(pass && "No mappable, coherent memory");

            res = vkAllocateMemory(pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemory);
            assert(res == VK_SUCCESS);

            res = vkBindImageMemory(pDevice->GetDevice(), m_pTexture2D, m_deviceMemory, 0);
            assert(res == VK_SUCCESS);
        }

        // Create the Image View
        //
        {
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image = m_pTexture2D;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = VK_FORMAT_R8G8B8A8_UNORM;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;
            res = vkCreateImageView(pDevice->GetDevice(), &info, NULL, &m_pTextureSRV);
            assert(res == VK_SUCCESS);
        }

        // Tell ImGUI what the image view is
        //
        io.Fonts->TexID = (void *)m_pTextureSRV;

        // Allocate memory int upload heap and copy the texture into it
        //
        char *ptr = (char*)pUploadHeap->Suballocate(upload_size, 512);

        memcpy(ptr, pixels, width*height * 4);

        // Copy from upload heap into the vid mem image
        //
        {
            VkImageMemoryBarrier copy_barrier[1] = {};
            copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copy_barrier[0].image = m_pTexture2D;
            copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy_barrier[0].subresourceRange.levelCount = 1;
            copy_barrier[0].subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(pUploadHeap->GetCommandList(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = width;
            region.imageExtent.height = height;
            region.imageExtent.depth = 1;
            vkCmdCopyBufferToImage(pUploadHeap->GetCommandList(), pUploadHeap->GetResource(), m_pTexture2D, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier use_barrier[1] = {};
            use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier[0].image = m_pTexture2D;
            use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            use_barrier[0].subresourceRange.levelCount = 1;
            use_barrier[0].subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(pUploadHeap->GetCommandList(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
        }

        // Kick off the upload
        //
        pUploadHeap->FlushAndFinish();

        // Create sampler
        //
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_sampler);
            assert(res == VK_SUCCESS);
        }

        // Vertex shader
        //
        const char *vertShaderTextGLSL =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform vertexBuffer {\n"
            "    mat4 ProjectionMatrix;\n"
            "} myVertexBuffer;\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 1) in vec2 inTexCoord;\n"
            "layout (location = 2) in vec4 inColor;\n"
            "layout (location = 0) out vec2 outTexCoord;\n"
            "layout (location = 1) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   outTexCoord = inTexCoord;\n"
            "   gl_Position = myVertexBuffer.ProjectionMatrix * pos;\n"
            "}\n";


        // Pixel shader
        //
        const char *fragShaderTextGLSL =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec2 inTexCoord;\n"
            "layout (location = 1) in vec4 inColor;\n"
            "\n"
            "layout (location = 0) out vec4 outColor;\n"
            "\n"
            "layout(set=0, binding=1) uniform texture2D sTexture;\n"
            "layout(set=0, binding=2) uniform sampler sSampler;\n"
            "\n"
            "void main() {\n"
            "#if 1\n"
            "   outColor = inColor * texture(sampler2D(sTexture, sSampler), inTexCoord.st);\n"
            "#else\n"
            "   outColor = inColor;\n"
            "#endif\n"
            "}\n";

        const char* fragShaderTextHLSL =
            "\n"
            "[[vk::binding(1, 0)]] Texture2D texture0;\n"
            "[[vk::binding(2, 0)]] SamplerState sampler0;\n"
            "\n"
            "[[vk::location(0)]] float4 main(\n"
            "         [[vk::location(0)]] in float2 uv : TEXCOORD, \n"
            "         [[vk::location(1)]] in float4 col: COLOR)\n"
            ": SV_Target\n"
            "{\n"
            "#if 1\n"
            "   return col * texture0.Sample(sampler0, uv.xy); \n"
            "#else\n"
            "   return col;\n"
            "#endif\n"
            "}";

        // Compile and create shaders
        //
        DefineList defines;
        VkPipelineShaderStageCreateInfo m_vertexShader, m_fragmentShader;

        res = VKCompileFromString(pDevice->GetDevice(), SST_GLSL, VK_SHADER_STAGE_VERTEX_BIT, vertShaderTextGLSL, "main", &defines, &m_vertexShader);
        assert(res == VK_SUCCESS);

#define USE_GLSL 1
#ifdef USE_GLSL 
        res = VKCompileFromString(pDevice->GetDevice(), SST_GLSL, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderTextGLSL, "main", &defines, &m_fragmentShader);
        assert(res == VK_SUCCESS);
#else

        res = VKCompileFromString(pDevice->GetDevice(), SST_HLSL, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderTextHLSL, "main", &defines, &m_fragmentShader);
        assert(res == VK_SUCCESS);
#endif
        m_shaderStages.clear();
        m_shaderStages.push_back(m_vertexShader);
        m_shaderStages.push_back(m_fragmentShader);

        // Create descriptor sets
        //
        VkDescriptorSetLayoutBinding layout_bindings[3];
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[0].pImmutableSamplers = NULL;

        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[1].pImmutableSamplers = NULL;

        layout_bindings[2].binding = 2;
        layout_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        layout_bindings[2].descriptorCount = 1;
        layout_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[2].pImmutableSamplers = NULL;

        // Next take layout bindings and use them to create a descriptor set layout
        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = NULL;
        descriptor_layout.flags = 0;
        descriptor_layout.bindingCount = 3;
        descriptor_layout.pBindings = layout_bindings;


        res = vkCreateDescriptorSetLayout(pDevice->GetDevice(), &descriptor_layout, NULL, &m_desc_layout);
        assert(res == VK_SUCCESS);

        /* Now use the descriptor layout to create a pipeline layout */
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = NULL;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &m_desc_layout;

        res = vkCreatePipelineLayout(pDevice->GetDevice(), &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
        assert(res == VK_SUCCESS);

        // Create descriptor pool, allocate and update the descriptors
        std::vector<VkDescriptorPoolSize> type_count = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 128 },{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128 }, { VK_DESCRIPTOR_TYPE_SAMPLER, 128 } };

        VkDescriptorPoolCreateInfo descriptor_pool = {};
        descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool.pNext = NULL;
        descriptor_pool.flags = 0;
        descriptor_pool.maxSets = 128 * 3;
        descriptor_pool.poolSizeCount = (uint32_t)type_count.size();
        descriptor_pool.pPoolSizes = type_count.data();

        res = vkCreateDescriptorPool(pDevice->GetDevice(), &descriptor_pool, NULL, &m_descriptorPool);
        assert(res == VK_SUCCESS);

        VkDescriptorSetAllocateInfo alloc_info[1];
        alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info[0].pNext = NULL;
        alloc_info[0].descriptorPool = m_descriptorPool;
        alloc_info[0].descriptorSetCount = 1;
        alloc_info[0].pSetLayouts = &m_desc_layout;

        for (int i = 0; i < 128; i++)
        {
            res = vkAllocateDescriptorSets(pDevice->GetDevice(), alloc_info, &m_descriptorSet[i]);
            assert(res == VK_SUCCESS);

            // update descriptor set, uniforms part
            m_pConstBuf->SetDescriptorSet(0, sizeof(VERTEX_CONSTANT_BUFFER), m_descriptorSet[i]);

            // update descriptor set, image and sample part
            VkDescriptorImageInfo desc_image[1] = {};
            desc_image[0].sampler = m_sampler;
            desc_image[0].imageView = m_pTextureSRV;
            desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet writes[2];
            writes[0] = {};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].pNext = NULL;
            writes[0].dstSet = m_descriptorSet[i];
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writes[0].pImageInfo = desc_image;
            writes[0].dstArrayElement = 0;
            writes[0].dstBinding = 1;

            writes[1] = {};
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].pNext = NULL;
            writes[1].dstSet = m_descriptorSet[i];
            writes[1].descriptorCount = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            writes[1].pImageInfo = desc_image;
            writes[1].dstArrayElement = 0;
            writes[1].dstBinding = 2;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), 2, writes, 0, NULL);
        }

        UpdatePipeline(renderPass);
    }

    //--------------------------------------------------------------------------------------
    //
    // UpdatePipeline
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::UpdatePipeline(VkRenderPass renderPass)
    {
        if (renderPass == VK_NULL_HANDLE)
            return;

        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_pDevice->GetDevice(), m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }

        // vertex input state
        //
        VkVertexInputBindingDescription vi_binding = {};
        vi_binding.binding = 0;
        vi_binding.stride = sizeof(ImDrawVert);
        vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vi_attrs[3] =
        {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2 },
            { 2, 0, VK_FORMAT_R8G8B8A8_UNORM, sizeof(float) * 4 }
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

        VkPipelineColorBlendAttachmentState att_state[1] = {};
        att_state[0].blendEnable = VK_TRUE;
        att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        att_state[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        // Color blend state

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

        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.pNext = NULL;
        vp.flags = 0;
        vp.viewportCount = 1;
        vp.scissorCount = 1;
        vp.pScissors = NULL;
        vp.pViewports = NULL;

        // depth stencil state

        VkPipelineDepthStencilStateCreateInfo ds;
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext = NULL;
        ds.flags = 0;
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
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

        VkPipelineMultisampleStateCreateInfo ms;
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.pNext = NULL;
        ms.flags = 0;
        ms.pSampleMask = NULL;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
        pipeline.pStages = m_shaderStages.data();
        pipeline.stageCount = (uint32_t)m_shaderStages.size();
        pipeline.renderPass = renderPass;
        pipeline.subpass = 0;

        VkResult res = vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &m_pipeline);
        assert(res == VK_SUCCESS);
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::OnDestroy()
    {
        if (!m_pDevice)
            return;

        vkDestroyImageView(m_pDevice->GetDevice(), m_pTextureSRV, NULL);

        // Our heaps dot allow freeing descriptosets, also this incurrs in driver overhead
        //
        //vkFreeDescriptorSets(m_pDevice->GetDevice(), m_descriptorPool, 1, &m_descriptorSet);

        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_desc_layout, nullptr);
        m_desc_layout = VK_NULL_HANDLE;

        vkDestroyPipeline(m_pDevice->GetDevice(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;

        vkDestroyDescriptorPool(m_pDevice->GetDevice(), m_descriptorPool, NULL);
        m_descriptorPool = VK_NULL_HANDLE;

        vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, NULL);
        m_deviceMemory = VK_NULL_HANDLE;

        vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_pipelineLayout, NULL);
        m_pipelineLayout = VK_NULL_HANDLE;

        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, NULL);
        m_sampler = VK_NULL_HANDLE;

        vkDestroyImage(m_pDevice->GetDevice(), m_pTexture2D, NULL);
        m_pTexture2D = VK_NULL_HANDLE;
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void ImGUI::Draw(VkCommandBuffer cmd_buf)
    {
        SetPerfMarkerBegin(cmd_buf, "ImGUI");

        ImGui::Render();

        if (m_pipeline == VK_NULL_HANDLE)
            return;


        ImDrawData* draw_data = ImGui::GetDrawData();

        // Create and grow vertex/index buffers if needed
        char *pVertices = NULL;
        VkDescriptorBufferInfo VerticesView;
        m_pConstBuf->AllocVertexBuffer(draw_data->TotalVtxCount, sizeof(ImDrawVert), (void **)&pVertices, &VerticesView);

        char *pIndices = NULL;
        VkDescriptorBufferInfo IndicesView;
        m_pConstBuf->AllocIndexBuffer(draw_data->TotalIdxCount, sizeof(ImDrawIdx), (void **)&pIndices, &IndicesView);

        ImDrawVert* vtx_dst = (ImDrawVert*)pVertices;
        ImDrawIdx* idx_dst = (ImDrawIdx*)pIndices;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }

        // Setup orthographic projection matrix into our constant buffer
        VkDescriptorBufferInfo ConstantBufferGpuDescriptor;
        {
            VERTEX_CONSTANT_BUFFER* constant_buffer;
            m_pConstBuf->AllocConstantBuffer(sizeof(VERTEX_CONSTANT_BUFFER), (void **)&constant_buffer, &ConstantBufferGpuDescriptor);

            float L = 0.0f;
            float R = ImGui::GetIO().DisplaySize.x;
            float B = ImGui::GetIO().DisplaySize.y;
            float T = 0.0f;
            float mvp[4][4] =
            {
                { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
            };
            memcpy(constant_buffer->mvp, mvp, sizeof(mvp));
        }

        // Setup viewport
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = ImGui::GetIO().DisplaySize.y;
        viewport.width = ImGui::GetIO().DisplaySize.x;
        viewport.height = -(float)(ImGui::GetIO().DisplaySize.y);
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &VerticesView.buffer, &VerticesView.offset);
        vkCmdBindIndexBuffer(cmd_buf, IndicesView.buffer, IndicesView.offset, VK_INDEX_TYPE_UINT16);

        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        uint32_t uniformOffsets[1] = { (uint32_t)ConstantBufferGpuDescriptor.offset };
        ImTextureID texID = NULL;

        // Render command lists
        int vtx_offset = 0;
        int idx_offset = 0;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    VkRect2D scissor;
                    scissor.offset.x = (uint32_t)pcmd->ClipRect.x;
                    scissor.offset.y = (uint32_t)pcmd->ClipRect.y;
                    scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                    scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

                    // get a new descriptor (from the 128 we allocated) and update it with the texture we want to use for rendering.
                    if (texID != pcmd->TextureId)
                    {
                        texID = pcmd->TextureId;

                        VkDescriptorImageInfo desc_image[1] = {};
                        desc_image[0].sampler = m_sampler;
                        desc_image[0].imageView = (VkImageView)pcmd->TextureId;
                        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkWriteDescriptorSet writes[2];
                        writes[0] = {};
                        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writes[0].pNext = NULL;
                        writes[0].dstSet = m_descriptorSet[m_currentDescriptorIndex];
                        writes[0].descriptorCount = 1;
                        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        writes[0].pImageInfo = desc_image;
                        writes[0].dstArrayElement = 0;
                        writes[0].dstBinding = 1;

                        writes[1] = {};
                        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writes[1].pNext = NULL;
                        writes[1].dstSet = m_descriptorSet[m_currentDescriptorIndex];
                        writes[1].descriptorCount = 1;
                        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                        writes[1].pImageInfo = desc_image;
                        writes[1].dstArrayElement = 0;
                        writes[1].dstBinding = 2;

                        vkUpdateDescriptorSets(m_pDevice->GetDevice(), 2, writes, 0, NULL);

                        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet[m_currentDescriptorIndex], 1, uniformOffsets);

                        m_currentDescriptorIndex++;
                        m_currentDescriptorIndex &= 127;
                    }

                    vkCmdDrawIndexed(cmd_buf, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
                }
                idx_offset += pcmd->ElemCount;
            }
            vtx_offset += cmd_list->VtxBuffer.Size;
        }
        
        SetPerfMarkerEnd(cmd_buf);
    }
}