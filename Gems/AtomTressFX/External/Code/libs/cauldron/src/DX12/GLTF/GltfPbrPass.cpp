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
#include "GltfPbrPass.h"
#include "Misc/ThreadPool.h"
#include "GltfHelpers.h"
#include "Base/ShaderCompilerHelper.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnCreate(
        Device *pDevice,
        UploadHeap* pUploadHeap,
        ResourceViewHeaps *pHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
        SkyDome *pSkyDome,
        bool bUseShadowMask,
        DXGI_FORMAT outFormat,
        uint32_t sampleCount)
    {
        m_sampleCount = sampleCount;
        m_pResourceViewHeaps = pHeaps;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

        m_outFormat = outFormat;

        const json &j3 = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

        /////////////////////////////////////////////
        // Load BRDF look up table for the PBR shader

        m_BrdfLut.InitFromFile(pDevice, pUploadHeap, "BrdfLut.dds", false); // LUT images are stored as linear

        // Create default material
        //
        {
            m_defaultMaterial.m_pbrMaterialParameters.m_doubleSided = false;
            m_defaultMaterial.m_pbrMaterialParameters.m_blending = false;

            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_emissiveFactor = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_baseColorFactor = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_metallicRoughnessValues = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            m_defaultMaterial.m_pbrMaterialParameters.m_params.m_specularGlossinessFactor = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

            std::map<std::string, Texture *> texturesBase;
            CreateGPUMaterialData(&m_defaultMaterial, texturesBase, pSkyDome);
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
            std::map<std::string, Texture *> texturesBase;
            for (auto const& value : textureIds)
                texturesBase[value.first] = m_pGLTFTexturesAndBuffers->GetTextureViewByID(value.second);

            CreateGPUMaterialData(tfmat, texturesBase, pSkyDome);
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

                    // Gets the geometry topology (so far we are not doing anything with this)
                    //
                    int32_t mode = GetElementInt(primitive, "mode", 4);

                    // Defines for the shader compiler, they will hold the PS and VS bindings for the geometry, io and textures 
                    //
                    DefineList attributeDefines;

                    // Set input layout from glTF attributes and set VS bindings
                    //
                    const json::object_t &attribute = primitive["attributes"];

                    std::vector<tfAccessor> vertexBuffers(attribute.size());
                    std::vector<std::string> semanticNames(attribute.size());
                    std::vector<D3D12_INPUT_ELEMENT_DESC> layout(attribute.size());

                    uint32_t cnt = 0;
                    for (auto it = attribute.begin(); it != attribute.end(); it++, cnt++)
                    {
                        const json::object_t &accessor = accessors[it->second];

                        // let the compiler know we have this stream
                        attributeDefines[std::string("HAS_") + it->first] = "1";

                        // split semantic name from index, DX doesnt like the trailing number
                        uint32_t semanticIndex = 0;
                        SplitGltfAttribute(it->first, &semanticNames[cnt], &semanticIndex);

                        // Create Input Layout
                        //
                        layout[cnt].SemanticName = semanticNames[cnt].c_str(); // we need to set it in the pipeline function (because of multithreading)
                        layout[cnt].SemanticIndex = semanticIndex;
                        layout[cnt].Format = GetFormat(accessor.at("type"), accessor.at("componentType"));
                        layout[cnt].InputSlot = (UINT)cnt;
                        layout[cnt].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                        layout[cnt].InstanceDataStepRate = 0;
                        layout[cnt].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

                        // Get VB accessors
                        //
                        m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetBufferDetails(it->second, &vertexBuffers[cnt]);
                    }

                    // Get Index and vertex buffer buffer accessors and create the geometry
                    //
                    tfAccessor indexBuffer;
                    m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetBufferDetails(primitive["indices"], &indexBuffer);
                    m_pGLTFTexturesAndBuffers->CreateGeometry(indexBuffer, vertexBuffers, &pPrimitive->m_geometry);

                    // Create the descriptors, the root signature and the pipeline
                    //
                    {
                        bool bUsingSkinning = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;
                        CreateDescriptors(pDevice->GetDevice(), bUsingSkinning, &attributeDefines, pPrimitive);
                        CreatePipeline(pDevice->GetDevice(), semanticNames, layout, &attributeDefines, pPrimitive);
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateGPUMaterialData
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateGPUMaterialData(PBRMaterial *tfmat, std::map<std::string, Texture *> &texturesBase, SkyDome *pSkyDome)
    {
        uint32_t cnt = 0;

        // count the number of textures to init bindings and descriptor
        {
            tfmat->m_textureCount = (int)texturesBase.size();

            tfmat->m_textureCount += 1;       // This is for the BRDF LUT texture

            if (pSkyDome)
                tfmat->m_textureCount += 2;   // +2 because the skydome has a specular, diffusse and BDRF maps
        }

        // Alloc descriptor layout and init the descriptor set 
        if (tfmat->m_textureCount >= 0)
        {
            // allocate descriptor table for the textures
            m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(tfmat->m_textureCount, &tfmat->m_texturesTable);

            //create SRVs and #defines for the BRDF LUT resources
            tfmat->m_pbrMaterialParameters.m_defines["ID_brdfTexture"] = std::to_string(cnt);
            CreateSamplerForBrdfLut(cnt, &tfmat->m_samplers[cnt]);
            m_BrdfLut.CreateSRV(cnt, &tfmat->m_texturesTable);
            cnt++;

            //create SRVs and #defines for the IBL resources
            if (pSkyDome)
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_diffuseCube"] = std::to_string(cnt);
                pSkyDome->SetDescriptorDiff(cnt, &tfmat->m_texturesTable, cnt, &tfmat->m_samplers[cnt]);
                cnt++;

                tfmat->m_pbrMaterialParameters.m_defines["ID_specularCube"] = std::to_string(cnt);
                pSkyDome->SetDescriptorSpec(cnt, &tfmat->m_texturesTable, cnt, &tfmat->m_samplers[cnt]);
                cnt++;

                tfmat->m_pbrMaterialParameters.m_defines["USE_IBL"] = "1";
            }

            // Create SRVs and #defines so the shader compiler knows what the index of each texture is
            for (auto it = texturesBase.begin(); it != texturesBase.end(); it++)
            {
                tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it->first] = std::to_string(cnt);
                it->second->CreateSRV(cnt, &tfmat->m_texturesTable);
                CreateSamplerForPBR(cnt, &tfmat->m_samplers[cnt]);
                cnt++;
            }
        }

        // Allocate the slot for looking up the shadow buffer
        //
        assert(cnt <= 9);   // 10th slot is reserved for shadow buffer
        tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(9);
        CreateSamplerForShadowMap(9, &tfmat->m_samplers[cnt]);
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
                pPrimitive->m_PipelineRender->Release();
                pPrimitive->m_RootSignature->Release();
            }
        }

        m_BrdfLut.OnDestroy();
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptors for a combination of material and geometry
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateDescriptors(ID3D12Device* pDevice, bool bUsingSkinning, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive)
    {
        CD3DX12_DESCRIPTOR_RANGE DescRange[2];
        DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, pPrimitive->m_pMaterial->m_textureCount, 0); // texture table
        DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9);                                       // shadow buffer

        int params = 0;
        CD3DX12_ROOT_PARAMETER RTSlot[5];

        // b0 <- Constant buffer 'per frame'
        RTSlot[params++].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

        // textures table
        if (pPrimitive->m_pMaterial->m_textureCount > 0)
        {
            RTSlot[params++].InitAsDescriptorTable(1, &DescRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
        }

        // shadow buffer
        RTSlot[params++].InitAsDescriptorTable(1, &DescRange[1], D3D12_SHADER_VISIBILITY_PIXEL);

        // b1 <- Constant buffer 'per object', these are mainly the material data
        RTSlot[params++].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

        // b2 <- Constant buffer holding the skinning matrices
        if (bUsingSkinning)
        {
            RTSlot[params++].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(2);
        }

        // the root signature contains up to 5 slots to be used
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
        descRootSignature.pParameters = RTSlot;
        descRootSignature.NumParameters = params;
        descRootSignature.pStaticSamplers = pPrimitive->m_pMaterial->m_samplers;
        descRootSignature.NumStaticSamplers = pPrimitive->m_pMaterial->m_textureCount + 1;  // account for shadow sampler

        // deny uneccessary access to certain pipeline stages
        descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
            | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ID3DBlob *pOutBlob, *pErrorBlob = NULL;
        ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
        ThrowIfFailed(pDevice->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&pPrimitive->m_RootSignature)));
        SetName(pPrimitive->m_RootSignature, "GltfPbr::m_RootSignature");

        pOutBlob->Release();
        if (pErrorBlob)
            pErrorBlob->Release();
    }

    //--------------------------------------------------------------------------------------
    //
    // CreatePipeline
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreatePipeline(ID3D12Device* pDevice, std::vector<std::string> semanticNames, std::vector<D3D12_INPUT_ELEMENT_DESC> layout, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive)
    {
        /////////////////////////////////////////////
        // Compile and create shaders

        D3D12_SHADER_BYTECODE shaderVert, shaderPixel;
        {
            // Create #defines based on material properties and vertex attributes 
            DefineList defines = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines + (*pAttributeDefines);

            CompileShaderFromFile("GLTFPbrPass-VS.hlsl", &defines, "mainVS", "vs_5_0", 0, &shaderVert);
            CompileShaderFromFile("GLTFPbrPass-PS.hlsl", &defines, "mainPS", "ps_5_0", 0, &shaderPixel);
        }

        // Set blending
        //
        CD3DX12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        blendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC
        {
            (pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines.Has("DEF_alphaMode_BLEND")),
            FALSE,
            D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        }; 

        /////////////////////////////////////////////
        // Create a PSO description

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
        descPso.InputLayout = { layout.data(), (UINT)layout.size() };
        descPso.pRootSignature = pPrimitive->m_RootSignature;
        descPso.VS = shaderVert;
        descPso.PS = shaderPixel;
        descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descPso.RasterizerState.CullMode = (pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_doubleSided) ? D3D12_CULL_MODE_NONE : D3D12_CULL_MODE_FRONT;
        descPso.BlendState = blendState;
        descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        descPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        descPso.SampleMask = UINT_MAX;
        descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descPso.NumRenderTargets = 1;
        descPso.RTVFormats[0] = m_outFormat;
        descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        descPso.SampleDesc.Count = m_sampleCount;
        descPso.NodeMask = 0;

        ThrowIfFailed(
            pDevice->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&pPrimitive->m_PipelineRender))
        );
        SetName(pPrimitive->m_PipelineRender, "GltfPbrPass::m_PipelineRender");
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::Draw(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pShadowBufferSRV)
    {
        UserMarker marker(pCommandList, "gltfPBR");

        struct Transparent
        {
            float m_depth;
            PBRPrimitives *m_pPrimitive;
            D3D12_GPU_VIRTUAL_ADDRESS m_perFrameDesc;
            D3D12_GPU_VIRTUAL_ADDRESS m_perObjectDesc;
            D3D12_GPU_VIRTUAL_ADDRESS m_pPerSkeleton;
            operator float() { return -m_depth; }
        };

        std::vector<Transparent> m_transparent;

        // Set descriptor heaps
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap(), m_pResourceViewHeaps->GetSamplerHeap() };
        pCommandList->SetDescriptorHeaps(2, pDescriptorHeaps);

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
            D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

            // loop through primitives
            //
            PBRMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if (pPrimitive->m_PipelineRender == NULL)
                    continue;

                // Set per Object constants
                //
                per_object *cbPerObject;
                D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc;
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
                    pPrimitive->DrawPrimitive(pCommandList, pShadowBufferSRV, m_pGLTFTexturesAndBuffers->GetPerFrameConstants(), perObjectDesc, pPerSkeleton);
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
                    t.m_perFrameDesc = m_pGLTFTexturesAndBuffers->GetPerFrameConstants();
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
            t.m_pPrimitive->DrawPrimitive(pCommandList, pShadowBufferSRV, t.m_perFrameDesc, t.m_perObjectDesc, t.m_pPerSkeleton);
        }
    }

    void PBRPrimitives::DrawPrimitive(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pShadowBufferSRV, D3D12_GPU_VIRTUAL_ADDRESS perFrameDesc, D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc, D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton)
    {
        // Bind indices and vertices using the right offsets into the buffer
        //
        pCommandList->IASetIndexBuffer(&m_geometry.m_IBV);
        pCommandList->IASetVertexBuffers(0, (UINT)m_geometry.m_VBV.size(), m_geometry.m_VBV.data());

        // Bind Descriptor sets
        //
        pCommandList->SetGraphicsRootSignature(m_RootSignature);
        int paramIndex = 0;

        // bind the per scene constant buffer descriptor
        pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, perFrameDesc);

        // bind the textures and samplers descriptors
        if (m_pMaterial->m_textureCount > 0)
        {
            pCommandList->SetGraphicsRootDescriptorTable(paramIndex++, m_pMaterial->m_texturesTable.GetGPU());
        }

        // bind the shadow buffer
        pCommandList->SetGraphicsRootDescriptorTable(paramIndex++, pShadowBufferSRV->GetGPU());

        // bind the per object constant buffer descriptor
        pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, perObjectDesc);

        // bind the skeleton bind matrices constant buffer descriptor
        if (pPerSkeleton != 0)
            pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, pPerSkeleton);

        // Bind Pipeline
        //
        pCommandList->SetPipelineState(m_PipelineRender);

        // Draw
        //
        pCommandList->DrawIndexedInstanced(m_geometry.m_NumIndices, 1, 0, 0, 0);
    }
}