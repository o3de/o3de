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
#include "GltfHelpers.h"
#include "Base/ShaderCompilerHelper.h"
#include "Base/ResourceViewHeaps.h"
#include "Base/ExtDebugMarkers.h"
#include "Base/Helper.h"
#include "Misc/Cache.h"

#include "GltfDepthPass.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfDepthPass::OnCreate(
        Device *pDevice,
        VkRenderPass renderPass,
        UploadHeap* pUploadHeap,
        ResourceViewHeaps *pHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers)
    {
        m_pDevice = pDevice;
        m_renderPass = renderPass;

        m_pResourceViewHeaps = pHeaps;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

        const json &j3 = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

        /////////////////////////////////////////////
        // Create default material

        m_defaultMaterial.m_textureCount = 0;
        m_defaultMaterial.m_descriptorSet = VK_NULL_HANDLE;
        m_defaultMaterial.m_descriptorSetLayout = VK_NULL_HANDLE;
        m_defaultMaterial.m_doubleSided = false;

        // Create static sampler in case there is transparency
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
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_sampler);
            assert(res == VK_SUCCESS);
        }

        // Create materials (in a depth pass materials are still needed to handle non opaque textures
        //    
        if (j3.find("materials") != j3.end())
        {
            json::array_t materials = j3["materials"];

            m_materialsData.resize(materials.size());
            for (uint32_t i = 0; i < materials.size(); i++)
            {
                json::object_t material = materials[i];

                DepthMaterial *tfmat = &m_materialsData[i];

                // Load material constants. This is a depth pass and we are only interested in the mask texture
                //               
                tfmat->m_doubleSided = GetElementBoolean(material, "doubleSided", false);
                std::string alphaMode = GetElementString(material, "alphaMode", "OPAQUE");
                tfmat->m_defines["DEF_alphaMode_" + alphaMode] = std::to_string(1);

                // If transparent use the baseColorTexture for alpha
                //
                if (alphaMode == "MASK")
                {
                    tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));

                    auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
                    if (pbrMetallicRoughnessIt != material.end())
                    {
                        const json::object_t &pbrMetallicRoughness = pbrMetallicRoughnessIt->second;

                        int id = GetElementInt(pbrMetallicRoughness, "baseColorTexture/index", -1);
                        if (id >= 0)
                        {
                            // allocate descriptor table for the texture
                            tfmat->m_textureCount = 1;
                            m_pResourceViewHeaps->AllocDescriptor(tfmat->m_textureCount, &m_sampler, &tfmat->m_descriptorSetLayout, &tfmat->m_descriptorSet);
                            VkImageView textureView = pGLTFTexturesAndBuffers->GetTextureViewByID(id);
                            SetDescriptorSet(m_pDevice->GetDevice(), 0, textureView, &m_sampler, tfmat->m_descriptorSet);
                            tfmat->m_defines["ID_baseColorTexture"] = "0";
                            tfmat->m_defines["ID_baseTexCoord"] = std::to_string(GetElementInt(pbrMetallicRoughness, "baseColorTexture/texCoord", 0));
                        }
                    }
                }
            }
        }

        // Load Meshes
        //
        if (j3.find("meshes") != j3.end())
        {
            const json::array_t &meshes = j3["meshes"];
            const json::array_t &accessors = j3["accessors"];

            m_meshes.resize(meshes.size());
            for (uint32_t i = 0; i < meshes.size(); i++)
            {
                DepthMesh *tfmesh = &m_meshes[i];
                const json::array_t &primitives = meshes[i]["primitives"];
                tfmesh->m_pPrimitives.resize(primitives.size());

                for (uint32_t p = 0; p < primitives.size(); p++)
                {
                    json::object_t primitive = primitives[p];
                    DepthPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

                    // Set Material
                    //
                    auto mat = primitive.find("material");
                    if (mat != primitive.end())
                        pPrimitive->m_pMaterial = &m_materialsData[mat->second];
                    else
                        pPrimitive->m_pMaterial = &m_defaultMaterial;

                    bool isTransparent = pPrimitive->m_pMaterial->m_defines.find("DEF_alphaMode_OPAQUE") == pPrimitive->m_pMaterial->m_defines.end();

                    // Defines for the shader compiler, they will hold the PS and VS bindings for the geometry, io and textures 
                    //
                    DefineList attributeDefines;

                    // Set input layout from glTF attributes and set VS bindings
                    //
                    std::vector<tfAccessor> vertexBuffers;
                    std::vector<VkVertexInputAttributeDescription> layout;

                    const json::object_t &attribute = primitive["attributes"];
                    for (auto it = attribute.begin(); it != attribute.end(); it++)
                    {
                        std::string semanticName = it->first;

                        // For the depth pass we are only interested in a few attributes
                        //
                        if (
                            (semanticName == "POSITION") || // for obvious reasons
                            ((isTransparent == true) && (semanticName == "TEXCOORD_0")) ||  // in case the material is transparent
                            (semanticName.substr(0, 7) == "WEIGHTS") || // for skinning
                            (semanticName.substr(0, 6) == "JOINTS") // for skinning
                            )
                        {
                            const json::object_t &accessor = accessors[it->second];

                            // Get VB accessors
                            //
                            tfAccessor vertexBuffer;
                            m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetBufferDetails(it->second, &vertexBuffer);
                            vertexBuffers.push_back(vertexBuffer);

                            // let the compiler know we have this stream
                            attributeDefines[std::string("ID_4VS_") + it->first] = std::to_string(layout.size());

                            // Create Input Layout
                            //
                            VkVertexInputAttributeDescription l;
                            l.location = (uint32_t)layout.size();
                            l.format = GetFormat(accessor.at("type"), accessor.at("componentType"));
                            l.offset = 0;
                            l.binding = (uint32_t)layout.size();
                            layout.push_back(l);
                        }
                    }

                    // Get Index and vertex buffer buffer accessors and create the geometry
                    //
                    tfAccessor indexBuffer;
                    pGLTFTexturesAndBuffers->m_pGLTFCommon->GetBufferDetails(primitive["indices"], &indexBuffer);
                    pGLTFTexturesAndBuffers->CreateGeometry(indexBuffer, vertexBuffers, &pPrimitive->m_Geometry);

                    // Set PS bindings
                    {
                        if (isTransparent)
                        {
                            attributeDefines[std::string("ID_4PS_TEXCOORD_0")] = std::to_string(0);
                        }
                    }

                    // Create Pipeline
                    //
                    {
                        int skinId = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                        int inverseMatrixBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinId);
                        CreateDescriptors(pDevice, inverseMatrixBufferSize, &attributeDefines, pPrimitive);
                        CreatePipeline(pDevice, layout, &attributeDefines, pPrimitive);
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void GltfDepthPass::OnDestroy()
    {
        for (uint32_t m = 0; m < m_meshes.size(); m++)
        {
            DepthMesh *pMesh = &m_meshes[m];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                DepthPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];
                vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->m_pipeline, nullptr);
                pPrimitive->m_pipeline = VK_NULL_HANDLE;
                vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->m_pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->m_descriptorSetLayout, NULL);
                m_pResourceViewHeaps->FreeDescriptor(pPrimitive->m_descriptorSet);
            }
        }

        for (int i = 0; i < m_materialsData.size(); i++)
        {
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_materialsData[i].m_descriptorSetLayout, NULL);
            m_pResourceViewHeaps->FreeDescriptor(m_materialsData[i].m_descriptorSet);
        }

        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptors for a combination of material and geometry
    //
    //--------------------------------------------------------------------------------------
    void GltfDepthPass::CreateDescriptors(Device *pDevice, int inverseMatrixBufferSize, DefineList *pAttributeDefines, DepthPrimitives *pPrimitive)
    {
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layout_bindings[0].pImmutableSamplers = NULL;
        (*pAttributeDefines)["ID_PER_FRAME"] = std::to_string(layout_bindings[0].binding);

        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layout_bindings[1].pImmutableSamplers = NULL;
        (*pAttributeDefines)["ID_PER_OBJECT"] = std::to_string(layout_bindings[1].binding);

        if (inverseMatrixBufferSize > 0)
        {
            VkDescriptorSetLayoutBinding b;

            // skinning matrices
            b.binding = 2;
            b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            b.descriptorCount = 1;
            b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            b.pImmutableSamplers = NULL;
            (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(b.binding);

            layout_bindings.push_back(b);
        }

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &pPrimitive->m_descriptorSetLayout, &pPrimitive->m_descriptorSet);

        // set descriptors entries

        m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(per_frame), pPrimitive->m_descriptorSet);
        m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(per_object), pPrimitive->m_descriptorSet);

        if (inverseMatrixBufferSize > 0)
        {
            m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitive->m_descriptorSet);
        }

        std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pPrimitive->m_descriptorSetLayout };
        if (pPrimitive->m_pMaterial->m_descriptorSetLayout != VK_NULL_HANDLE)
            descriptorSetLayout.push_back(pPrimitive->m_pMaterial->m_descriptorSetLayout);

        /////////////////////////////////////////////
        // Create a PSO description

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = NULL;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
        pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
        pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();

        VkResult res = vkCreatePipelineLayout(m_pDevice->GetDevice(), &pPipelineLayoutCreateInfo, NULL, &pPrimitive->m_pipelineLayout);
        assert(res == VK_SUCCESS);

    }
        
    //--------------------------------------------------------------------------------------
    //
    // CreatePipeline
    //
    //--------------------------------------------------------------------------------------
    void GltfDepthPass::CreatePipeline(Device *pDevice, std::vector<VkVertexInputAttributeDescription> layout, DefineList *pAttributeDefines, DepthPrimitives *pPrimitive)
    {
        /////////////////////////////////////////////
        // Compile and create shaders

        VkPipelineShaderStageCreateInfo vertexShader, fragmentShader = {};
        {
            // Create #defines based on material properties and vertex attributes 
            DefineList defines = pPrimitive->m_pMaterial->m_defines + (*pAttributeDefines);

            VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, "GLTFDepthPass-vert.glsl", "main", &defines, &vertexShader);
            VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFDepthPass-frag.glsl", "main", &defines, &fragmentShader);
        }
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

        /////////////////////////////////////////////
        // Create a Pipeline 

        // vertex input state

        std::vector<VkVertexInputBindingDescription> vi_binding(layout.size());
        for (int i = 0; i < layout.size(); i++)
        {
            vi_binding[i].binding = layout[i].binding;
            vi_binding[i].stride = SizeOfFormat(layout[i].format);
            vi_binding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        VkPipelineVertexInputStateCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = NULL;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = (uint32_t)vi_binding.size();
        vi.pVertexBindingDescriptions = vi_binding.data();
        vi.vertexAttributeDescriptionCount = (uint32_t)layout.size();
        vi.pVertexAttributeDescriptions = layout.data();

        // input assembly state

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
        rs.cullMode = pPrimitive->m_pMaterial->m_doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;;
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

        VkPipelineColorBlendStateCreateInfo cb;
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.flags = 0;
        cb.pNext = NULL;
        cb.attachmentCount = 0; //set to 1 when transparency
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
        ds.depthTestEnable = true;
        ds.depthWriteEnable = true;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        ds.back.failOp = VK_STENCIL_OP_KEEP;
        ds.back.passOp = VK_STENCIL_OP_KEEP;
        ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
        ds.back.compareMask = 0;
        ds.back.reference = 0;
        ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.back.writeMask = 0;
        ds.depthBoundsTestEnable = VK_FALSE;
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
        pipeline.layout = pPrimitive->m_pipelineLayout;
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
        pipeline.renderPass = m_renderPass;
        pipeline.subpass = 0;

        VkResult res = vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pPrimitive->m_pipeline);
        assert(res == VK_SUCCESS);
    }

    //--------------------------------------------------------------------------------------
    //
    // SetPerFrameConstants
    //
    //--------------------------------------------------------------------------------------
    GltfDepthPass::per_frame *GltfDepthPass::SetPerFrameConstants()
    {
        GltfDepthPass::per_frame *cbPerFrame;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(GltfDepthPass::per_frame), (void **)&cbPerFrame, &m_perFrameDesc);

        return cbPerFrame;
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void GltfDepthPass::Draw(VkCommandBuffer cmd_buf)
    {
        SetPerfMarkerBegin(cmd_buf, "DepthPass");

        // loop through nodes
        //
        std::vector<tfNode> *pNodes = &m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_nodes;
        XMMATRIX *pNodesMatrices = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_pCurrentFrameTransformedData->m_worldSpaceMats.data();

        for (uint32_t i = 0; i < pNodes->size(); i++)
        {
            tfNode *pNode = &pNodes->at(i);
            if ((pNode == NULL) || (pNode->meshIndex < 0))
                continue;

            // skinning matrices constant buffer
            VkDescriptorBufferInfo *pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

            DepthMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (int p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                DepthPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if (pPrimitive->m_pipeline == VK_NULL_HANDLE)
                    continue;

                // Set per Object constants
                //
                per_object *cbPerObject;
                VkDescriptorBufferInfo perObjectDesc;
                m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), (void **)&cbPerObject, &perObjectDesc);
                cbPerObject->mWorld = pNodesMatrices[i];

                // Bind indices and vertices using the right offsets into the buffer
                //
                Geometry *pGeometry = &pPrimitive->m_Geometry;
                for (uint32_t i = 0; i < pGeometry->m_VBV.size(); i++)
                {
                    vkCmdBindVertexBuffers(cmd_buf, i, 1, &pGeometry->m_VBV[i].buffer, &pGeometry->m_VBV[i].offset);
                }
                
                vkCmdBindIndexBuffer(cmd_buf, pGeometry->m_IBV.buffer, pGeometry->m_IBV.offset, pGeometry->m_indexType);

                // Bind Descriptor sets
                //
                VkDescriptorSet descriptorSets[2] = { pPrimitive->m_descriptorSet, pPrimitive->m_pMaterial->m_descriptorSet };
                uint32_t descritorSetCount = 1 + pPrimitive->m_pMaterial->m_textureCount;

                uint32_t uniformOffsets[3] = { (uint32_t)m_perFrameDesc.offset,  (uint32_t)perObjectDesc.offset, (pPerSkeleton) ? (uint32_t)pPerSkeleton->offset : 0 };
                uint32_t uniformOffsetsCount = (pPerSkeleton) ? 3 : 2;

                vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pPrimitive->m_pipelineLayout, 0, descritorSetCount, descriptorSets, uniformOffsetsCount, uniformOffsets);

                // Bind Pipeline
                //
                vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pPrimitive->m_pipeline);

                // Draw
                //
                vkCmdDrawIndexed(cmd_buf, pGeometry->m_NumIndices, 1, 0, 0, 0);
            }
        }

        SetPerfMarkerEnd(cmd_buf);
    }
}
