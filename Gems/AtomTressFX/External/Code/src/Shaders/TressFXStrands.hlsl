//---------------------------------------------------------------------------------------
// Shader code related to hair strands in the graphics pipeline.
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

#include "TressFXUtilities.hlsl"

//--------------------------------------------------------------------------------------
//
//      Structured buffers with hair vertex info
//      Accessors to allow changes to how they are accessed.
//
//--------------------------------------------------------------------------------------
[[vk::binding(0, 1)]] StructuredBuffer<float4> g_GuideHairVertexPositions : register(t0, space1);
[[vk::binding(1, 1)]] StructuredBuffer<float4> g_GuideHairVertexTangents : register(t1, space1);

[[vk::binding(0, 0)]] StructuredBuffer<float> g_HairThicknessCoeffs : register(t0, space0);
[[vk::binding(1, 0)]] StructuredBuffer<float2> g_HairStrandTexCd : register(t1, space0);
[[vk::binding(2, 0)]] Texture2D<float4> BaseAlbedoTexture : register(t2, space0);

[[vk::binding(0, 4)]] SamplerState      LinearWrapSampler : register(s0, space4);


inline float4 GetPosition(int index)
{
    return g_GuideHairVertexPositions[index];
}
inline float4 GetTangent(int index)
{
    return g_GuideHairVertexTangents[index];
}
inline float GetThickness(int index)
{
    return g_HairThicknessCoeffs[index];
}

float3 GetStrandColor(int index, float fractionOfStrand)
{
    float3 rootColor;
    float3 tipColor;

    float2 texCd = g_HairStrandTexCd[index / NumVerticesPerStrand].xy;
    rootColor = BaseAlbedoTexture.SampleLevel(LinearWrapSampler, texCd, 0).rgb;
    tipColor  = MatTipColor.rgb;

    // Multiply with Base Material color
    rootColor *= MatBaseColor.rgb;

    // Update the color based on position along the strand (vertex level) and lerp between tip and root if within the tipPercentage requested
    float rootRange = 1.f - TipPercentage;
    return (fractionOfStrand > rootRange) ? lerp(rootColor, tipColor, fractionOfStrand) : rootColor;
}

struct TressFXVertex
{
    float4 Position;
    float4 Tangent;
    float4 p0p1;
    float4 StrandColor;
};

TressFXVertex GetExpandedTressFXVert(uint vertexId, float3 eye, float2 winSize, float4x4 viewProj)
{

    // Access the current line segment
    uint index = vertexId / 2;  // vertexId is actually the indexed vertex id when indexed triangles are used
                                // Get updated positions and tangents from simulation result
    float3 v = g_GuideHairVertexPositions[index].xyz;
    float3 t = g_GuideHairVertexTangents[index].xyz;

    // Get hair strand thickness
    uint indexInStrand = index % NumVerticesPerStrand;
    float fractionOfStrand = (float)indexInStrand / (NumVerticesPerStrand - 1);
    float ratio = (EnableThinTip > 0) ? lerp(1.0, FiberRatio, fractionOfStrand) : 1.0;  //need length of full strand vs the length of this point on the strand. 	

    // Calculate right and projected right vectors
    float3 right = Safe_normalize(cross(t, Safe_normalize(v - eye)));
    float2 proj_right = Safe_normalize(MatrixMult(viewProj, float4(right, 0)).xy);

    // We always to to expand for faster hair AA, we may want to gauge making this adjustable
    float expandPixels = 0.71;

    // Calculate the negative and positive offset screenspace positions
    float4 hairEdgePositions[2]; // 0 is negative, 1 is positive
    hairEdgePositions[0] = float4(v + -1.0 * right * ratio * FiberRadius, 1.0);
    hairEdgePositions[1] = float4(v + 1.0 * right * ratio * FiberRadius, 1.0);
    hairEdgePositions[0] = MatrixMult(viewProj, hairEdgePositions[0]);
    hairEdgePositions[1] = MatrixMult(viewProj, hairEdgePositions[1]);

    // Gonna hi-jack Tangent.w (unused) and add a .w component to strand color to store a strand UV
    float2 strandUV;
    strandUV.x = (vertexId & 0x01) ? 0.f : 1.f;
    strandUV.y = fractionOfStrand;

    // Write output data
    TressFXVertex Output = (TressFXVertex)0;
    float fDirIndex = (vertexId & 0x01) ? -1.0 : 1.0;
    Output.Position = ((vertexId & 0x01) ? hairEdgePositions[0] : hairEdgePositions[1]) + fDirIndex * float4(proj_right * expandPixels / winSize.y, 0.0f, 0.0f) * ((vertexId & 0x01) ? hairEdgePositions[0].w : hairEdgePositions[1].w);
    Output.Tangent = float4(t, strandUV.x);
    Output.p0p1 = float4(hairEdgePositions[0].xy / max(hairEdgePositions[0].w, TRESSFX_FLOAT_EPSILON), hairEdgePositions[1].xy / max(hairEdgePositions[1].w, TRESSFX_FLOAT_EPSILON));
    Output.StrandColor = float4(GetStrandColor(index, fractionOfStrand), strandUV.y);
    return Output;
}

TressFXVertex GetExpandedTressFXShadowVert(uint vertexId, float3 eye, float2 winSize, float4x4 viewProj)
{

    // Access the current line segment
    uint index = vertexId / 2;  // vertexId is actually the indexed vertex id when indexed triangles are used
                                // Get updated positions and tangents from simulation result
    float3 v = g_GuideHairVertexPositions[index].xyz;
    float3 t = g_GuideHairVertexTangents[index].xyz;

    // Get hair strand thickness
    uint indexInStrand = index % NumVerticesPerStrand;
    float fractionOfStrand = (float)indexInStrand / (NumVerticesPerStrand - 1);
    float ratio = (EnableThinTip > 0) ? lerp(1.0, FiberRatio, fractionOfStrand) : 1.0;  //need length of full strand vs the length of this point on the strand. 	

    // Calculate right and projected right vectors
    float3 right = Safe_normalize(cross(t, Safe_normalize(v - eye)));
    float2 proj_right = Safe_normalize(MatrixMult(viewProj, float4(right, 0)).xy);

    // We always to to expand for faster hair AA, we may want to gauge making this adjustable
    float expandPixels = 1.f; // Disable for shadows 0.71;

    // Calculate the negative and positive offset screenspace positions
    float4 hairEdgePositions[2]; // 0 is negative, 1 is positive
    hairEdgePositions[0] = float4(v + -1.0 * right * ratio * FiberRadius, 1.0);
    hairEdgePositions[1] = float4(v + 1.0 * right * ratio * FiberRadius, 1.0);
    hairEdgePositions[0] = MatrixMult(viewProj, hairEdgePositions[0]);
    hairEdgePositions[1] = MatrixMult(viewProj, hairEdgePositions[1]);

    // Write output data
    TressFXVertex Output = (TressFXVertex)0;
    float fDirIndex = (vertexId & 0x01) ? -1.0 : 1.0;
    Output.Position = ((vertexId & 0x01) ? hairEdgePositions[0] : hairEdgePositions[1]) + fDirIndex * float4(proj_right * expandPixels / winSize.y, 0.0f, 0.0f) * ((vertexId & 0x01) ? hairEdgePositions[0].w : hairEdgePositions[1].w);
    return Output;
}

// EndHLSL

