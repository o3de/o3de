//-------------------------------------------------------------------------------------
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

#ifndef THREAD_GROUP_SIZE
#define THREAD_GROUP_SIZE 64
#endif

// Adi: this shader file is only required for applying physics and and debug render.  
// If physics is not applied (during first iterations) it is not used and can be skipped.

[[vk::binding(3, 0)]] cbuffer ConstBufferCS_BoneMatrix : register(b3, space0)
{
    float4 cColor;
    float4 vLightDir;
    int4 g_NumMeshVertices;
    float4x4 mMW;
    float4x4 mMVP;
    row_major float4x4 g_BoneSkinningMatrix[AMD_TRESSFX_MAX_NUM_BONES];
}

struct BoneSkinningData
{
    float4 boneIndex; // x, y, z and w component are four bone indices per strand
    float4 boneWeight; // x, y, z and w component are four bone weights per strand
};

struct StandardVertex
{
    float4 position;
    float4 normal;
};

// UAVs		
[[vk::binding(0, 0)]] RWStructuredBuffer<StandardVertex> bs_collMeshVertexPositions : register(u0, space0);

// SRVs
[[vk::binding(1, 0)]] StructuredBuffer<BoneSkinningData> bs_boneSkinningData : register(t1, space0);
[[vk::binding(2, 0)]] StructuredBuffer<StandardVertex> bs_initialVertexPositions : register(t2, space0);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void BoneSkinning(uint GIndex : SV_GroupIndex, uint3 GId : SV_GroupID, uint3 DTid : SV_DispatchThreadID)
{
    uint local_id = GIndex;
    uint group_id = GId.x;
    uint global_id = local_id + group_id * THREAD_GROUP_SIZE;

    if (global_id >= g_NumMeshVertices.x)
        return;

    float3 pos = bs_initialVertexPositions[global_id].position.xyz;
    float3 n = bs_initialVertexPositions[global_id].normal.xyz;

    // compute a bone skinning transform
    BoneSkinningData skinning = bs_boneSkinningData[global_id];

    // Interpolate world space bone matrices using weights. 
    row_major float4x4 bone_matrix = g_BoneSkinningMatrix[skinning.boneIndex[0]] * skinning.boneWeight[0];
    float weight_sum = skinning.boneWeight[0];

    // Each vertex gets influence from four bones. In case there are less than four bones, boneIndex and boneWeight would be zero. 
    // This number four was set in Maya exporter and also used in loader. So it should not be changed unless you have a very strong reason and are willing to go through all spots. 
    for (int i = 1; i < 4; i++)
    {
        if (skinning.boneWeight[i] > 0)
        {
            bone_matrix += g_BoneSkinningMatrix[skinning.boneIndex[i]] * skinning.boneWeight[i];
            weight_sum += skinning.boneWeight[i];
        }
    }

    bone_matrix /= weight_sum;

    pos.xyz = mul(float4(pos.xyz, 1), bone_matrix).xyz;
    n.xyz = mul(float4(n.xyz, 0), bone_matrix).xyz;
    bs_collMeshVertexPositions[global_id].position.xyz = pos;
    bs_collMeshVertexPositions[global_id].normal.xyz = n;
}

// visualization

struct VsOutput
{
    float4 vPositionSS : SV_POSITION;
    float3 vPos_ : vPos_;
    float3 vNormal_ : vNormal_;
};

VsOutput BoneSkinningVisualizationVS( uint vertexId : SV_VertexID )
{
    VsOutput output;

    output.vNormal_ = mul(bs_collMeshVertexPositions[vertexId].normal.xyz, (float3x3)mMW);
    float4 inputVertexPos = float4(bs_collMeshVertexPositions[vertexId].position.xyz, 1.0);

    output.vPositionSS = mul(mMVP, float4(inputVertexPos.xyz, 1.0f));
    output.vPos_ = inputVertexPos.xyz;
    return output;
}

struct PSOutput
{
    float4 vColor : SV_TARGET;
};

PSOutput BoneSkinningVisualizationPS(VsOutput input) : SV_Target
{
    PSOutput output = (PSOutput)0;
    float4 vDiffuse = float4(cColor.xyz, 1.0);
    float fAmbient = 0.2;
    float3 vLightDir1 = float3(-1., 0., -1.);

    //float fLighting = saturate(dot(normalize(vLightDir), input.vNormal_));
    float fLighting = saturate(dot(normalize(vLightDir), input.vNormal_)) + 0.7*saturate(dot(normalize(vLightDir1), input.vNormal_));
    fLighting = max(fLighting, fAmbient);

    output.vColor = vDiffuse * fLighting;
    return output;
}

//EndHLSL