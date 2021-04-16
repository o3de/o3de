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
#include "Misc/Cache.h"
#include "GltfHelpers.h"
#include "Base/Helper.h"
#include "Base/ShaderCompilerHelper.h"
#include "Base/ExtDebugMarkers.h"
#include "PostProc/Skydome.h"

#include "GltfPbrPass.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnCreate(
        Device* pDevice,
        VkRenderPass renderPass,
        UploadHeap* pUploadHeap,
        ResourceViewHeaps *pHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
        SkyDome *pSkyDome,
        VkImageView ShadowMapView,
        VkSampleCountFlagBits sampleCount
    )
    {        
        m_pDevice = pDevice;
        m_renderPass = renderPass;
        m_sampleCount = sampleCount;
        m_pResourceViewHeaps = pHeaps;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

        const json &j3 = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

        /////////////////////////////////////////////
        // Load BRDF look up table for the PBR shader

        m_brdfLutTexture.InitFromFile(pDevice, pUploadHeap, "BrdfLut.dds", false); // LUT images are stored as linear
        m_brdfLutTexture.CreateSRV(&m_brdfLutView);

        /////////////////////////////////////////////
        // Create Samplers

        //for pbr materials
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = 0;
            info.maxLod = 10000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplerPbr);
            assert(res == VK_SUCCESS);
        }

        // specular BRDF lut sampler
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
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_brdfLutSampler);
            assert(res == VK_SUCCESS);
        }

        // shadowmap sampler
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_NEAREST;
            info.minFilter = VK_FILTER_NEAREST;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplerShadow);
            assert(res == VK_SUCCESS);
        }

        // Create a default material that is all black.
        //
        {
            m_defaultMaterial.m_pbrMaterialParameters.m_doubleSided = false;
            m_defaultMaterial.m_pbrMaterialParameters.m_blending = false;

            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_emissiveFactor = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_baseColorFactor = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_metallicRoughnessValues = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_specularGlossinessFactor = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

            std::map<std::string, VkImageView> texturesBase;
            CreateGPUMaterialData(&m_defaultMaterial, texturesBase, pSkyDome, ShadowMapView);
        }

        // Load PBR 2.0 Materials
        //
        const json::array_t &materials = j3["materials"];
        m_materialsData.resize(materials.size());
        for (uint32_t i = 0; i < materials.size(); i++)
        {
            PBRMaterial *tfmat = &m_materialsData[i];

            // Get PBR material parameters and texture IDs
            //
            std::map<std::string, int> textureIds;
            ProcessMaterials(materials[i], &tfmat->m_pbrMaterialParameters, textureIds);

            // translate texture IDs into textureViews
            //
            std::map<std::string, VkImageView> texturesBase;
            for (auto const& value : textureIds)
                texturesBase[value.first] = m_pGLTFTexturesAndBuffers->GetTextureViewByID(value.second);

            CreateGPUMaterialData(tfmat, texturesBase, pSkyDome, ShadowMapView);
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
                PBRMesh *tfmesh = &m_meshes[i];
                const json::array_t &primitives = meshes[i]["primitives"];
                tfmesh->m_pPrimitives.resize(primitives.size());

                for (uint32_t p = 0; p < primitives.size(); p++)
                {
                    json::object_t primitive = primitives[p];
                    PBRPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

                    // Sets primitive's material, or set a default material if none was specified in the GLTF
                    //
                    auto mat = primitive.find("material");
                    pPrimitive->m_pMaterial = (mat != primitive.end()) ? &m_materialsData[mat->second] : &m_defaultMaterial;

                    int32_t mode = GetElementInt(primitive, "mode", 4);

                    // Defines for the shader compiler, they will hold the PS and VS bindings for the geometry, io and textures 
                    //
                    DefineList attributeDefines;

                    // Set input layout from glTF attributes and set VS bindings
                    //
                    const json::object_t &attribute = primitive["attributes"];
                    std::vector<tfAccessor> vertexBuffers(attribute.size());
                    std::vector<VkVertexInputAttributeDescription> layout;
                    {
                        uint32_t in = 0;
                        for (auto it = attribute.begin(); it != attribute.end(); it++, in++)
                        {
                            const json::object_t &accessor = accessors[it->second];

                            // let the compiler know we have this stream
                            attributeDefines[std::string("ID_4VS_") + it->first] = std::to_string(in);

                            // Create Input Layout
                            //
                            VkVertexInputAttributeDescription l;
                            l.location = (uint32_t)in;
                            l.format = GetFormat(accessor.at("type"), accessor.at("componentType"));
                            l.offset = 0;
                            l.binding = in;
                            layout.push_back(l);

                            // Get VB accessors
                            //
                            m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetBufferDetails(it->second, &vertexBuffers[in]);
                        }

                        // Get Index and vertex buffer buffer accessors and create the geometry
                        //
                        tfAccessor indexBuffer;
                        m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetBufferDetails(primitive["indices"], &indexBuffer);
                        m_pGLTFTexturesAndBuffers->CreateGeometry(indexBuffer, vertexBuffers, &pPrimitive->m_geometry);
                    }

                    // Set PS bindings
                    {
                        uint32_t out = 0;
                        std::vector<std::string> attributeList = { "POSITION", "COLOR_0", "TEXCOORD_0", "TEXCOORD_1", "NORMAL", "TANGENT" };
                        for (auto it = attributeList.begin(); it != attributeList.end(); it++)
                        {
                            auto att = attribute.find(*it);
                            if (att == attribute.end())
                                continue;

                            attributeDefines[std::string("ID_4PS_") + *it] = std::to_string(out);
                            out++;
                        }

                        attributeDefines[std::string("ID_4PS_WORLDPOS")] = std::to_string(out);
                        attributeDefines[std::string("ID_4PS_LASTID")] = std::to_string(out+1);
                    }

                    // Create descriptors and pipelines
                    //
                    {
                        int skinId = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                        int inverseMatrixBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinId);
                        CreateDescriptors(pDevice, inverseMatrixBufferSize, &attributeDefines, pPrimitive);
                        CreatePipeline(pDevice, layout, &attributeDefines, pPrimitive);
                    };
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateGPUMaterialData
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateGPUMaterialData(PBRMaterial *tfmat, std::map<std::string, VkImageView> &texturesBase, SkyDome *pSkyDome, VkImageView ShadowMapView)
    {
        // count the number of textures to init bindings and descriptor
        {
            tfmat->m_textureCount = (int)texturesBase.size();

            tfmat->m_textureCount += 1;   // This is for the BRDF LUT texture

            if (pSkyDome)
                tfmat->m_textureCount += 2;   // +2 because the skydome has a specular, diffusse and BDRF maps

            if (ShadowMapView != VK_NULL_HANDLE) // will use a shadowmap texture if present
                tfmat->m_textureCount += 1;
        }

        // Alloc descriptor layout and init the descriptor set 
        if (tfmat->m_textureCount >= 0)
        {
            // allocate descriptor table for the textures
            m_pResourceViewHeaps->AllocDescriptor(tfmat->m_textureCount, NULL, &tfmat->m_descriptorLayout, &tfmat->m_texturesDescriptorSet);

            uint32_t cnt = 0;

            //create SRVs and #defines for the BRDF LUT resources
            tfmat->m_pbrMaterialParameters.m_defines["ID_brdfTexture"] = std::to_string(cnt);
            SetDescriptorSet(m_pDevice->GetDevice(), cnt++, m_brdfLutView, &m_brdfLutSampler, tfmat->m_texturesDescriptorSet);

            //create SRVs and #defines for the IBL resources
            if (pSkyDome)
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_diffuseCube"] = std::to_string(cnt);
                pSkyDome->SetDescriptorDiff(cnt++, tfmat->m_texturesDescriptorSet);

                tfmat->m_pbrMaterialParameters.m_defines["ID_specularCube"] = std::to_string(cnt);
                pSkyDome->SetDescriptorSpec(cnt++, tfmat->m_texturesDescriptorSet);

                tfmat->m_pbrMaterialParameters.m_defines["USE_IBL"] = "1";
            }

            // Create SRV for the shadowmap
            if (ShadowMapView != VK_NULL_HANDLE)
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(cnt);
                SetDescriptorSet(m_pDevice->GetDevice(), cnt++, ShadowMapView, &m_samplerShadow, tfmat->m_texturesDescriptorSet);
            }

            // Create SRVs for the material textures
            for (auto it = texturesBase.begin(); it != texturesBase.end(); it++)
            {
                tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it->first] = std::to_string(cnt);
                SetDescriptorSet(m_pDevice->GetDevice(), cnt++, it->second, &m_samplerPbr, tfmat->m_texturesDescriptorSet);
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnDestroy()
    {
        for (uint32_t m = 0; m < m_meshes.size(); m++)
        {
            PBRMesh *pMesh = &m_meshes[m];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];
                vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->m_pipeline, nullptr);
                pPrimitive->m_pipeline = VK_NULL_HANDLE;
                vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->m_pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->m_descriptorLayout, NULL);
                m_pResourceViewHeaps->FreeDescriptor(pPrimitive->m_descriptorSet);
            }
        }

        for (int i = 0; i < m_materialsData.size(); i++)
        {
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_materialsData[i].m_descriptorLayout, NULL);
            m_pResourceViewHeaps->FreeDescriptor(m_materialsData[i].m_texturesDescriptorSet);
        }

        //destroy default material
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_defaultMaterial.m_descriptorLayout, NULL);
        m_pResourceViewHeaps->FreeDescriptor(m_defaultMaterial.m_texturesDescriptorSet);

        vkDestroySampler(m_pDevice->GetDevice(), m_samplerPbr, nullptr);
        vkDestroySampler(m_pDevice->GetDevice(), m_samplerShadow, nullptr);

        vkDestroyImageView(m_pDevice->GetDevice(), m_brdfLutView, NULL);
        vkDestroySampler(m_pDevice->GetDevice(), m_brdfLutSampler, nullptr);
        m_brdfLutTexture.OnDestroy();

    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptors for a combination of material and geometry
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateDescriptors(Device* pDevice, int inverseMatrixBufferSize, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive)
    {
        // Creates descriptor set layout binding for the constant buffers
        //
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);

        // Constant buffer 'per frame'
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].pImmutableSamplers = NULL;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        (*pAttributeDefines)["ID_PER_FRAME"] = std::to_string(layout_bindings[0].binding);

        // Constant buffer 'per object'
        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].pImmutableSamplers = NULL;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        (*pAttributeDefines)["ID_PER_OBJECT"] = std::to_string(layout_bindings[1].binding);

        // Constant buffer holding the skinning matrices
        if (inverseMatrixBufferSize >= 0)
        {
            VkDescriptorSetLayoutBinding b;

            // skinning matrices
            b.binding = 2;
            b.descriptorCount = 1;
            b.pImmutableSamplers = NULL;
            b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(b.binding);

            layout_bindings.push_back(b);
        }

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &pPrimitive->m_descriptorLayout, &pPrimitive->m_descriptorSet);

        // Init descriptors sets for the constant buffers
        //
        m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(per_frame), pPrimitive->m_descriptorSet);
        m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(per_object), pPrimitive->m_descriptorSet);

        if (inverseMatrixBufferSize >= 0)
        {
            m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitive->m_descriptorSet);
        }

        // Create the pipeline layout
        //
        std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pPrimitive->m_descriptorLayout };
        if (pPrimitive->m_pMaterial->m_descriptorLayout != VK_NULL_HANDLE)
            descriptorSetLayout.push_back(pPrimitive->m_pMaterial->m_descriptorLayout);

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
    void GltfPbrPass::CreatePipeline(Device* pDevice, std::vector<VkVertexInputAttributeDescription> layout, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive)
    {
        /////////////////////////////////////////////
        // Compile and create shaders

        VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
        {
            // Create #defines based on material properties and vertex attributes 
            DefineList defines = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines + (*pAttributeDefines);

            VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, "GLTFPbrPass-vert.glsl", "main", &defines, &vertexShader);
            VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFPbrPass-frag.glsl", "main", &defines, &fragmentShader);
        }
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

        /////////////////////////////////////////////
        // Create pipeline

        // vertex input state
        //
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
        rs.cullMode = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
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
        att_state[0].blendEnable = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines.Has("DEF_alphaMode_BLEND");
        att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

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
        ms.rasterizationSamples = m_sampleCount;
        ms.sampleShadingEnable = VK_FALSE;
        ms.alphaToCoverageEnable = VK_FALSE;
        ms.alphaToOneEnable = VK_FALSE;
        ms.minSampleShading = 0.0;

        // create pipeline 
        //
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
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::Draw(VkCommandBuffer cmd_buf)
    {
        SetPerfMarkerBegin(cmd_buf, "gltfPBR");

        struct Transparent
        {
            float m_depth;
            PBRPrimitives *m_pPrimitive;
            VkDescriptorBufferInfo m_perFrameDesc;
            VkDescriptorBufferInfo m_perObjectDesc;
            VkDescriptorBufferInfo *m_pPerSkeleton;
            operator float() { return -m_depth; }
        };

        std::vector<Transparent> m_transparent;

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

            // loop through primitives
            //
            PBRMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if (pPrimitive->m_pipeline == VK_NULL_HANDLE)
                    continue;

                // Set per Object constants from material
                //
                per_object *cbPerObject;
                VkDescriptorBufferInfo perObjectDesc;
                m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), (void **)&cbPerObject, &perObjectDesc);
                cbPerObject->mWorld = pNodesMatrices[i];
                PBRMaterialParameters *pPbrParams = &pPrimitive->m_pMaterial->m_pbrMaterialParameters;
                cbPerObject->m_pbrParams = pPbrParams->m_params;


                // Draw primitive
                //
                if (pPbrParams->m_blending == false)
                {
                    // If solid draw it 
                    //
                    pPrimitive->DrawPrimitive(cmd_buf, m_pGLTFTexturesAndBuffers->m_perFrameConstants, perObjectDesc, pPerSkeleton);
                }
                else
                {
                    // If transparent queue it for sorting
                    //
                    XMMATRIX mat = pNodesMatrices[i] * m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_perFrameData.mCameraViewProj;
                    XMVECTOR v = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p].m_center;

                    Transparent t;
                    t.m_depth = XMVectorGetW(XMVector4Transform(v, mat));
                    t.m_pPrimitive = pPrimitive;
                    t.m_perFrameDesc = m_pGLTFTexturesAndBuffers->m_perFrameConstants;
                    t.m_perObjectDesc = perObjectDesc;
                    t.m_pPerSkeleton = pPerSkeleton;

                    m_transparent.push_back(t);
                }
            }
        }

        // sort transparent primitives
        //
        std::sort(m_transparent.begin(), m_transparent.end());

        // Draw them sorted front to back
        //
        int tt = 0;
        for (auto &t : m_transparent)
        {
            t.m_pPrimitive->DrawPrimitive(cmd_buf, t.m_perFrameDesc, t.m_perObjectDesc, t.m_pPerSkeleton);
        }

        SetPerfMarkerEnd(cmd_buf);
    }

    void PBRPrimitives::DrawPrimitive(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo perFrameDesc, VkDescriptorBufferInfo perObjectDesc, VkDescriptorBufferInfo *pPerSkeleton)
    {
        // Bind indices and vertices using the right offsets into the buffer
        //
        for (uint32_t i = 0; i < m_geometry.m_VBV.size(); i++)
        {
            vkCmdBindVertexBuffers(cmd_buf, i, 1, &m_geometry.m_VBV[i].buffer, &m_geometry.m_VBV[i].offset);
        }

        vkCmdBindIndexBuffer(cmd_buf, m_geometry.m_IBV.buffer, m_geometry.m_IBV.offset, m_geometry.m_indexType);

        // Bind Descriptor sets
        //
        VkDescriptorSet descritorSets[2] = { m_descriptorSet, m_pMaterial->m_texturesDescriptorSet };
        uint32_t descritorSetsCount = (m_pMaterial->m_textureCount == 0) ? 1 : 2;

        uint32_t uniformOffsets[3] = { (uint32_t)perFrameDesc.offset,  (uint32_t)perObjectDesc.offset, (pPerSkeleton) ? (uint32_t)pPerSkeleton->offset : 0 };
        uint32_t uniformOffsetsCount = (pPerSkeleton) ? 3 : 2;

        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, descritorSetsCount, descritorSets, uniformOffsetsCount, uniformOffsets);

        // Bind Pipeline
        //
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Draw
        //
        vkCmdDrawIndexed(cmd_buf, m_geometry.m_NumIndices, 1, 0, 0, 0);
    }
}