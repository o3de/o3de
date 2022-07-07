// ----------------------------------------------------------------------------
// Compute-based skinning.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "TressFXBoneSkinning.h"

// TressFX
#include "EngineInterface.h"    // Engine and GLTF API-specific implementation pull ins
#include "TressFXHairObject.h"
#include "TressFXLayouts.h"
#include <iostream>
#include <fstream>
#include <string>

int static SuStringTokenizer(const std::string&    input,
                             const std::string&    delimiter,
                             std::vector<std::string>& results,
                             bool               includeEmpties)
{
    int iPos   = 0;
    int newPos = -1;
    int sizeS2 = (int)delimiter.length();
    int isize  = (int)input.length();

    if (isize == 0 || sizeS2 == 0)
        return 0;

    std::vector<int> positions;
    newPos = (int)input.find(delimiter, 0);

    if (newPos == std::string::npos)
        return 0;

    int numFound = 0;

    while (newPos >= iPos)
    {
        numFound++;
        positions.push_back(newPos);
        iPos   = newPos;
        newPos = (int)input.find(delimiter, iPos + sizeS2);
    }

    if (numFound == 0)
        return 0;

    for (int i = 0; i <= (int)positions.size(); ++i)
    {
        std::string s("");

        if (i == 0)
        {
            s = input.c_str() + i;
            s = s.substr(0, positions[i]);
        }
        else
        {
            int offset = positions[i - 1] + sizeS2;

            if (offset < isize)
            {
                if (i == (int)positions.size())
                    s = input.c_str() + offset;
                else if (i > 0)
                {
                    s = input.c_str() + positions[i - 1] + sizeS2;
                    s = s.substr(0, positions[i] - positions[i - 1] - sizeS2);
                }
            }
        }

        if (includeEmpties || (s.length() > 0))
            results.push_back(s);
    }

    return ++numFound;
}

bool TressFXBoneSkinning::LoadTressFXCollisionMeshData(
    EI_Scene * scene,
    const char* filePath,
    int skinNumber,
    const char* followBone)
{
    m_skinNumber = skinNumber;

    m_pTempIndices.clear();
    m_pTempNormals.clear();
    m_pTempVertices.clear();

    std::ifstream stream(filePath);

    if (!stream.is_open())
        return false;

    std::string          sLine;
    std::vector<std::string> sTokens;

    int numOfBones;

    std::vector<std::string> boneNames;
    while (std::getline(stream, sLine))
    {
        if (sLine.length() == 0)
            continue;

        // If # is in the very first column in the line, it is a comment.
        if (sLine[0] == '#')
            continue;

        sTokens.clear();
        int numFound = SuStringTokenizer(sLine, " ", sTokens, false);

        std::string token;

        if (numFound > 0)
        {
            token = sTokens[0];
        }
        else
        {
            token = sLine;
        }

        // load bone names.
        if (token.find("numOfBones") != std::string::npos)
        {
            numOfBones    = atoi(sTokens[1].c_str());
            int countBone = 0;

            while (1)
            {
                // next line
                std::getline(stream, sLine);

                if (sLine.length() == 0)
                    continue;

                // If # is in the very first column in the line, it is a comment.
                if (sLine[0] == '#')
                    continue;

                sTokens.clear();
                int numFound = SuStringTokenizer(sLine, " ", sTokens, false);
                boneNames.push_back(sTokens[1]);
                countBone++;

                if (countBone == numOfBones)
                    break;
            }
        }
        if (token.find("numOfVertices") != std::string::npos)  // load bone indices and weights for each strand
        {
            m_NumVertices    = (AMD::uint32)atoi(sTokens[1].c_str());
            boneSkinningData.resize(m_NumVertices);
            m_pTempVertices.resize(m_NumVertices);
            m_pTempNormals.resize(m_NumVertices);
            memset(boneSkinningData.data(), 0, sizeof(TressFXBoneSkinningData) * m_NumVertices);

            int index = 0;

            while (1)
            {
                // next line
                std::getline(stream, sLine);

                if (sLine.length() == 0)
                    continue;

                // If # is in the very first column in the line, it is a comment.
                if (sLine[0] == '#')
                    continue;

                sTokens.clear();
                int numFound = SuStringTokenizer(sLine, " ", sTokens, false);
                assert(numFound == 15);

                int vertexIndex = atoi(sTokens[0].c_str());
                assert(vertexIndex == index);

                AMD::float3& pos = m_pTempVertices[index];
                pos.x                     = (float)atof(sTokens[1].c_str());
                pos.y                     = (float)atof(sTokens[2].c_str());
                pos.z                     = (float)atof(sTokens[3].c_str());

                AMD::float3& normal = m_pTempNormals[index];
                normal.x                     = (float)atof(sTokens[4].c_str());
                normal.y                     = (float)atof(sTokens[5].c_str());
                normal.z                     = (float)atof(sTokens[6].c_str());

                TressFXBoneSkinningData skinData;


                int boneIndex   = atoi(sTokens[7].c_str());
                int engineIndex = scene->GetBoneIdByName(m_skinNumber, boneNames[boneIndex].c_str());
                skinData.boneIndex[0] = (float)engineIndex;

                boneIndex   = atoi(sTokens[8].c_str());
                engineIndex = scene->GetBoneIdByName(m_skinNumber, boneNames[boneIndex].c_str());
                skinData.boneIndex[1] = (float)engineIndex;

                boneIndex   = atoi(sTokens[9].c_str());
                engineIndex = scene->GetBoneIdByName(m_skinNumber, boneNames[boneIndex].c_str());
                skinData.boneIndex[2] = (float)engineIndex;

                boneIndex = atoi(sTokens[10].c_str());
                engineIndex = scene->GetBoneIdByName(m_skinNumber, boneNames[boneIndex].c_str());
                skinData.boneIndex[3] = (float)engineIndex;

                skinData.weight[0] = (float)atof(sTokens[11].c_str());
                skinData.weight[1] = (float)atof(sTokens[12].c_str());
                skinData.weight[2] = (float)atof(sTokens[13].c_str());
                skinData.weight[3] = (float)atof(sTokens[14].c_str());

                boneSkinningData[index] = skinData;

                ++index;

                if (index == m_NumVertices)
                    break;
            }
        }
        else if (token.find("numOfTriangles") != std::string::npos)  // triangle indices
        {
            m_NumTriangles = atoi(sTokens[1].c_str());
            int numIndices = m_NumTriangles * 3;
            m_pTempIndices.resize(numIndices);
            int index = 0;

            while (1)
            {
                // next line
                std::getline(stream, sLine);

                if (sLine.length() == 0)
                    continue;

                // If # is in the very first column in the line, it is a comment.
                if (sLine[0] == '#')
                    continue;

                sTokens.clear();
                int numFound = SuStringTokenizer(sLine, " ", sTokens, false);
                assert(numFound == 4);

                int triangleIndex = atoi(sTokens[0].c_str());
                assert(triangleIndex == index);

                m_pTempIndices[index * 3 + 0] = atoi(sTokens[1].c_str());
                m_pTempIndices[index * 3 + 1] = atoi(sTokens[2].c_str());
                m_pTempIndices[index * 3 + 2] = atoi(sTokens[3].c_str());


                ++index;

                if (index == m_NumTriangles)
                    break;
            }
        }
    }
    m_pScene = scene;
    m_followBone = scene->GetBoneIdByName(skinNumber, followBone);
    return true;
}

void TressFXBoneSkinning::Initialize(EI_RenderTargetSet * renderTargetSet, EI_Device*  pDevice,
                                       EI_CommandContext& commandContext, const char * name)
{
    // TODO: Following code should be moved to TransarencyPlugin.cpp.

    // load an effect for rendering
    EI_BindLayout * layouts[] = { GetBoneSkinningMeshLayout() };
    //std::vector<VkVertexInputAttributeDescription> inputs;

    EI_PSOParams psoParams;
    psoParams.depthTestEnable = true;
    psoParams.depthWriteEnable = true;
    psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

    psoParams.colorBlendParams.colorBlendEnabled = false;
    psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::Zero;
    psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::SrcColor;
    psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::Zero;
    psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::SrcAlpha;

    psoParams.layouts = layouts;
    psoParams.numLayouts = 1;
    psoParams.renderTargetSet = renderTargetSet;
    m_pRenderEffect = pDevice->CreateGraphicsPSO("TressFXBoneSkinning.hlsl", "BoneSkinningVisualizationVS", "TressFXBoneSkinning.hlsl", "BoneSkinningVisualizationPS", psoParams);

    // create a vertex and normal buffer
    size_t vertexBlockSize = GetSizeOfMeshElement();
    std::vector<AMD::uint8> dataVB((AMD::uint32)(vertexBlockSize * m_NumVertices)); //position + normal

    for (size_t i = 0; i < m_NumVertices; ++i)
    {
        // position
        memcpy(&dataVB[i * vertexBlockSize], &m_pTempVertices[i], sizeof(AMD::float3));

        // normal
        memcpy(&dataVB[i * vertexBlockSize + sizeof(AMD::float4)], &m_pTempNormals[i], sizeof(AMD::float3));
    }

    // create an index buffer
    m_numIndices = (int)m_pTempIndices.size();
    m_pIndexBuffer = pDevice->CreateBufferResource(sizeof(AMD::int32), m_numIndices, EI_BF_INDEXBUFFER, "IndexBuffer");
    commandContext.UpdateBuffer(m_pIndexBuffer.get(), m_pTempIndices.data());

    // UAV
    m_CollMeshVertPositionsUAV = pDevice->CreateBufferResource((int)vertexBlockSize, (int)m_NumVertices, EI_BF_NEEDSUAV, "CollMesh");

    EI_Barrier prepareMeshVertPositions[] =
    {
        { m_CollMeshVertPositionsUAV.get(), EI_STATE_UAV, EI_STATE_COPY_DEST }
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(prepareMeshVertPositions), prepareMeshVertPositions);
    commandContext.UpdateBuffer(m_CollMeshVertPositionsUAV.get(), dataVB.data());

    // SRV
    {
        m_BoneSkinningDataSRV = pDevice->CreateBufferResource(sizeof(TressFXBoneSkinningData), m_NumVertices, 0, "BoneSkinningData");
        commandContext.UpdateBuffer(m_BoneSkinningDataSRV.get(), boneSkinningData.data());

        boneSkinningData.resize(0);
    }

    {
        m_InitialVertexPositionsSRV = pDevice->CreateBufferResource((AMD::uint32)vertexBlockSize, m_NumVertices, 0, "InitialVertexPositions");
        commandContext.UpdateBuffer(m_InitialVertexPositionsSRV.get(), dataVB.data());
    }

    {
        m_TrimeshVertexIndicesSRV = pDevice->CreateBufferResource(sizeof(AMD::int32), (int)m_pTempIndices.size(), 0, "CSSkinningMeshIndices");
        commandContext.UpdateBuffer(m_TrimeshVertexIndicesSRV.get(), m_pTempIndices.data());
    }

    // constant buffer
    m_pUniformBuffer = pDevice->CreateBufferResource(sizeof(TressFXBoneSkinningUniformBuffer), 1, EI_BF_UNIFORMBUFFER, "TressFXBoneSkinningUniformBuffer");

    EI_Barrier finishUpload[] =
    {
        { m_CollMeshVertPositionsUAV.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
        { m_BoneSkinningDataSRV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_InitialVertexPositionsSRV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_TrimeshVertexIndicesSRV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(finishUpload), finishUpload);

    // Bind set
    EI_BindSetDescription bindSet = { { m_BoneSkinningDataSRV.get(), m_InitialVertexPositionsSRV.get(), m_CollMeshVertPositionsUAV.get(), m_pUniformBuffer.get() } };
    m_pBindSet = pDevice->CreateBindSet(GetBoneSkinningMeshLayout(), bindSet);

    // update bbox
    Vector3 center = Vector3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < m_pTempVertices.size(); ++i)
    {
        center = center + Vector3(&m_pTempVertices[i].x);
    }
    center = center / (float)m_pTempVertices.size();
    float radius = 0.0f;
    for (int i = 0; i < m_pTempVertices.size(); ++i) {
        Vector3 distance = Vector3(&m_pTempVertices[i].x) - center;
        radius = fmaxf(radius, distance.Length());
    }
    m_bbMin = { center.x - radius, center.y - radius, center.z - radius };
    m_bbMax = { center.x + radius, center.y + radius, center.z + radius };

    // release vertex and index arrays on CPU.
    m_pTempVertices.clear();
    m_pTempNormals.clear();
    m_pTempIndices.clear();

    // set color
    m_MeshColor = { 1.f, 0.0f, 0.0f };

    // load a compute shader
    m_pComputeEffectSkinning = pDevice->CreateComputeShaderPSO("TressFXBoneSkinning.hlsl", "BoneSkinning", layouts, 1);
}

Vector3 TressFXBoneSkinning::SkinPosition( int i )
{
    int global_id = 0;

    AMD::float3 vert = m_pTempVertices[global_id];
    XMVECTOR pos = { vert.x, vert.y, vert.z, 1.0f };
    AMD::float3 normal = m_pTempNormals[global_id];
    XMVECTOR n = { normal.x, normal.y, normal.z, 0.0f };

    // compute a bone skinning transform
    TressFXBoneSkinningData skinning = boneSkinningData[global_id];

    std::vector<XMMATRIX>& skinningMatrices = m_pScene->GetWorldSpaceSkeletonMats(m_skinNumber);
    // Interpolate world space bone matrices using weights.
    XMMATRIX bone_matrix = skinningMatrices[(int)skinning.boneIndex[0]] * skinning.weight[0];
    float weight_sum = skinning.weight[0];

    // Each vertex gets influence from four bones. In case there are less than four bones, boneIndex and boneWeight would be zero.
    // This number four was set in Maya exporter and also used in loader. So it should not be changed unless you have a very strong reason and are willing to go through all spots.
    for (int i = 1; i < 4; i++)
    {
        if (skinning.weight[i] > 0)
        {
            bone_matrix += skinningMatrices[(int)skinning.boneIndex[i]] * skinning.weight[i];
            weight_sum += skinning.weight[i];
        }
    }

    bone_matrix /= weight_sum;

    pos = XMVector4Transform( pos, bone_matrix);
    //n = mul(float4(n.xyz, 0), bone_matrix).xyz;
    return Vector3(0.0f, 0.0f, 0.0f);
}

void TressFXBoneSkinning::GetBoundingBox(Vector3& min, Vector3& max)
{
    XMMATRIX m = m_pScene->GetWorldSpaceSkeletonMats(m_skinNumber)[m_followBone]; // root matrix
    XMVECTOR minvec = { m_bbMin.x, m_bbMin.y, m_bbMin.z, 1.0f };
    XMVECTOR maxvec = { m_bbMax.x, m_bbMax.y, m_bbMax.z, 1.0f };
    XMVECTOR center = (maxvec + minvec) / 2.0;
    XMVECTOR newcenter = XMVector4Transform(center, m);

    minvec += newcenter - center;
    maxvec += newcenter - center;

    XMStoreFloat3((XMFLOAT3*)&min, minvec);
    XMStoreFloat3((XMFLOAT3*)&max, maxvec);
}

void TressFXBoneSkinning::GetInitialBoundingBox(Vector3& min, Vector3& max)
{
    min = m_bbMin;
    max = m_bbMax;
}

void TressFXBoneSkinning::Update(EI_CommandContext& commandContext, double fTime)
{
    if (!m_pComputeEffectSkinning || !m_pScene || !m_pScene->GetWorldSpaceSkeletonMats(m_skinNumber).size())
        return;

    EI_Marker marker(commandContext, "BoneSkinningUpdate");

    // update animation model before getting the skinning matrices.
    //m_pModel->UpdateObject(fTime);
    std::vector<XMMATRIX> boneMatrices = m_pScene->GetWorldSpaceSkeletonMats(m_skinNumber);
    const float*              pBoneMatricesInWS = (const float*)&boneMatrices[0];
  
    //-----------------------------
    // 1. BoneSkinning
    //-----------------------------
    //const SuArray<SuMatrix4>& m_BoneMatricesPerFrame = m_pModel->GetSkinningMatrices();

    for (int i = 0; i < boneMatrices.size(); ++i)
    {
        m_uniformBufferData.g_BoneSkinningMatrix[i] = *((AMD::float4x4*)&boneMatrices[i]);
    }
    m_uniformBufferData.g_NumMeshVertices = { (int)m_NumVertices, 0, 0, 0 };
    m_uniformBufferData.cColor = m_MeshColor;
    m_uniformBufferData.vLightDir = { 1.0f, 1.0f, 1.0f };
    m_uniformBufferData.mMW = m_pScene->GetMV();
    m_uniformBufferData.mMVP = m_pScene->GetMVP();

    commandContext.UpdateBuffer(m_pUniformBuffer.get(), &m_uniformBufferData);

    commandContext.BindPSO(m_pComputeEffectSkinning.get());
    EI_BindSet* bindSets[] = { m_pBindSet.get() };
    commandContext.BindSets(m_pComputeEffectSkinning.get(), 1, bindSets);

    // Run BoneSkinning
    {
        int numDispatchSize =
            (int)ceil((float)m_NumVertices / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
        commandContext.Dispatch(numDispatchSize);
        GetDevice()->GetTimeStamp("BoneSkinning");
    }

    // State transition for DX12
    EI_Barrier flushSkinnedVerts[] = { { m_CollMeshVertPositionsUAV.get(), EI_STATE_UAV, EI_STATE_UAV } };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(flushSkinnedVerts), flushSkinnedVerts);
}

// Adi: this is only the mesh debug render - not the actual skinned mesh
void TressFXBoneSkinning::DrawMesh(EI_CommandContext& commandContext)
{
    // draw the collision mesh
    if (m_NumVertices > 0)
    {
        EI_Marker marker(commandContext, "BoneSkinningDrawMesh");
    //    commandContext.BindPSO(m_pRenderEffect);
        EI_BindSet* bindSets[] = { m_pBindSet.get() };
        commandContext.BindSets(m_pRenderEffect.get(), 1, bindSets);
        EI_IndexedDrawParams drawParams;
        drawParams.numIndices = m_numIndices;
        drawParams.numInstances = 1;
        drawParams.pIndexBuffer = m_pIndexBuffer.get();
        commandContext.DrawIndexedInstanced(*m_pRenderEffect.get(), drawParams);
    }
}