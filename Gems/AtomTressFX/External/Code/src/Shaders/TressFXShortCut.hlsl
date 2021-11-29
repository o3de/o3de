//---------------------------------------------------------------------------------------
// Shader code related to TressFX Shortcut method
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

#include "TressFXRendering.hlsl"
#include "TressFXStrands.hlsl"

[[vk::binding(0, 2)]] cbuffer viewConstants : register(b0, space2)
{
    float4x4    g_mVP;
    float4      g_vEye;
    float4      g_vViewport;
    float4x4    cInvViewProjMatrix;
};

// Hair input structure to Pixel shaders
struct PS_INPUT_HAIR
{
    float4 Position    : SV_POSITION;
    float4 Tangent     : Tangent;
    float4 p0p1        : TEXCOORD0;
    float4 StrandColor : TEXCOORD1;
};

//////////////////////////////////////////////////////////////
// Hair Render VS (for depth alpha and color fill passes)
PS_INPUT_HAIR RenderHairDepthAlphaVS(uint vertexId : SV_VertexID)
{
    TressFXVertex tressfxVert =	GetExpandedTressFXVert(vertexId, g_vEye, g_vViewport.zw, g_mVP);

    PS_INPUT_HAIR Output;

    Output.Position     = tressfxVert.Position;
    Output.Tangent      = float4(1, 1, 1, tressfxVert.Tangent.w);   // Only need to preserve the Strand U coordinate as the tangent is not used in this pass. Operations should be compiled out
    Output.p0p1         = tressfxVert.p0p1;
    Output.StrandColor  = float4(1, 1, 1, tressfxVert.StrandColor.a); // Don't use the color as it's not needed. This way we can compile out the computation of strand color (Just need the Strand V coordinate)

    return Output;
}

PS_INPUT_HAIR RenderHairColorVS(uint vertexId : SV_VertexID)
{
    TressFXVertex tressfxVert = GetExpandedTressFXVert(vertexId, g_vEye, g_vViewport.zw, g_mVP);

    PS_INPUT_HAIR Output;

    Output.Position = tressfxVert.Position;
    Output.Tangent = tressfxVert.Tangent;
    Output.p0p1 = tressfxVert.p0p1;
    Output.StrandColor = tressfxVert.StrandColor;

    return Output;
}

[[vk::binding(0, 3)]] RWTexture2DArray<uint> RWFragmentDepthsTexture : register(u0, space3);

//////////////////////////////////////////////////////////////
// Depth Alpha PS
// First pass of ShortCut.
// Geometry pass that stores the 3 front fragment depths, and accumulates product of 1-alpha values in the render target.
[earlydepthstencil]
float DepthsAlphaPS(PS_INPUT_HAIR input) : SV_Target
{
    float3 vNDC = ScreenPosToNDC(input.Position.xyz, g_vViewport);
    float coverage = ComputeCoverage(input.p0p1.xy, input.p0p1.zw, vNDC.xy, g_vViewport.zw);
    float alpha = coverage * MatBaseColor.a;

    if (alpha < SHORTCUT_MIN_ALPHA)
        return 1.0;

    int2 vScreenAddress = int2(input.Position.xy);

    uint uDepth = asuint(input.Position.z);
    uint uDepth0Prev, uDepth1Prev;

    // Min of depth 0 and input depth
    // Original value is uDepth0Prev
    InterlockedMin(RWFragmentDepthsTexture[uint3(vScreenAddress, 0)], uDepth, uDepth0Prev);

    // Min of depth 1 and greater of the last compare
    // If fragment opaque, always use input depth (don't need greater depths)
    uDepth = (alpha > 0.98) ? uDepth : max(uDepth, uDepth0Prev);

    InterlockedMin(RWFragmentDepthsTexture[uint3(vScreenAddress, 1)], uDepth, uDepth1Prev);

    uint uDepth2Prev;

    // Min of depth 2 and greater of the last compare
    // If fragment opaque, always use input depth (don't need greater depths)
    uDepth = (alpha > 0.98) ? uDepth : max(uDepth, uDepth1Prev);

    InterlockedMin(RWFragmentDepthsTexture[uint3(vScreenAddress, 2)], uDepth, uDepth2Prev);

    return 1.0 - alpha;
}

//////////////////////////////////////////////////////////////
// Fullscreen Quad VS
struct VS_OUTPUT_SCREENQUAD
{
    float4 vPosition : SV_POSITION;
    float2 vTex      : TEXCOORD;
};

static const float2 Positions[] = { {-1, -1,}, {1, -1}, {-1, 1}, {1, 1} };

VS_OUTPUT_SCREENQUAD FullScreenVS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT_SCREENQUAD output;

    // Output vert positions
    output.vPosition = float4(Positions[vertexID].xy, 0, 1);

    // And tex-coords
#ifdef AMD_TRESSFX_DX12
    output.vTex = float2(Positions[vertexID].x, Positions[vertexID].y) * 0.5 + 0.5;
#else
    output.vTex = float2(Positions[vertexID].x, -Positions[vertexID].y) * 0.5 + 0.5;
#endif // AMD_TRESSFX_DX12

    return output;
}

//////////////////////////////////////////////////////////////
// Resolve Depth PS
[[vk::binding(0, 0)]] Texture2DArray<uint> FragmentDepthsTexture : register(t0, space0);

// Second pass of ShortCut.
// Full-screen pass that writes the farthest of the near depths for depth culling.
float ResolveDepthPS(VS_OUTPUT_SCREENQUAD input) : SV_Depth
{
    // Blend the layers of fragments from back to front
    int2 vScreenAddress = int2(input.vPosition.xy);

    // Write farthest depth value for culling in the next pass.
    // It may be the initial value of 1.0 if there were not enough fragments to write all depths, but then culling not important.
    uint uDepth = FragmentDepthsTexture[uint3(vScreenAddress, 2)];

    return asfloat(uDepth);
}

//////////////////////////////////////////////////////////////
// HairColorPS

#include "TressFXLighting.hlsl"

// If you change this, you MUST also change TressFXShadeParams in TressFXConstantBuffers.h and ShadeParams in TressFXPPLL.hlsl
struct ShadeParams
{
    // General information
    float       FiberRadius;
    // For deep approximated shadow lookup
    float       ShadowAlpha;
    float       FiberSpacing;
    // For lighting/shading
    float       HairEx2;
    float4		MatKValue;   // KAmbient, KDiffuse, KSpec1, Exp1
    float       HairKs2;
    float		fPadding0;
    float		fPadding1;
    float		fPadding2;
};

[[vk::binding(0, 5)]] cbuffer TressFXShadeParams  : register(b0, space5)
{
    ShadeParams HairParams[AMD_TRESSFX_MAX_HAIR_GROUP_RENDER];
};

float3 TressFXShading(float3 WorldPos, float3 vNDC, float3 vTangentCoverage, float2 uv, float2 strandUV, float3 baseColor, PS_INPUT_HAIR input)
{
    float3 vTangent = vTangentCoverage.xyz;
    float3 vPositionWS = WorldPos.xyz;
    float3 vViewDirWS = g_vEye - vPositionWS;

    HairShadeParams params;
    params.Color = baseColor;
    params.HairShadowAlpha = HairParams[RenderParamsIndex].ShadowAlpha;
    params.FiberRadius = HairParams[RenderParamsIndex].FiberRadius;
    params.FiberSpacing = HairParams[RenderParamsIndex].FiberSpacing;

    params.Ka = HairParams[RenderParamsIndex].MatKValue.x;
    params.Kd = HairParams[RenderParamsIndex].MatKValue.y;
    params.Ks1 = HairParams[RenderParamsIndex].MatKValue.z;
    params.Ex1 = HairParams[RenderParamsIndex].MatKValue.w;
    params.Ks2 = HairParams[RenderParamsIndex].HairKs2;
    params.Ex2 = HairParams[RenderParamsIndex].HairEx2;

    return AccumulateHairLight(vTangent, vPositionWS, vViewDirWS, params, vNDC);
}

// Third pass of ShortCut.
// Geometry pass that shades pixels passing early depth test.  Limited to near fragments due to previous depth write pass.
// Colors are accumulated in render target for a weighted average in final pass.
[earlydepthstencil]
float4 HairColorPS(PS_INPUT_HAIR input) : SV_Target
{
    // Strand Color read in is either the BaseMatColor, or BaseMatColor modulated with a color read from texture on vertex shader for base color;
    //along with modulation by the tip color  
    float4 strandColor = float4(input.StrandColor.rgb, MatBaseColor.a);

    // Grab the uv in case we need it
    float2 uv = float2(input.Tangent.w, input.StrandColor.w);

    // Apply StrandUVTiling
    // strandUV.y = (uv.y * StrandUVTilingFactor) % 1.f
    float2 strandUV = float2(uv.x, (uv.y * StrandUVTilingFactor) - floor(uv.y * StrandUVTilingFactor));

    // If we are supporting strand UV texturing, further blend in the texture color/alpha
    // Do this while computing NDC and coverage to hide latency from texture lookup
    if (EnableStrandUV)
        strandColor.rgb *= StrandAlbedoTexture.Sample(LinearWrapSampler, strandUV).rgb;

    float3 vNDC = ScreenPosToNDC(input.Position.xyz, g_vViewport);
    float coverage = ComputeCoverage(input.p0p1.xy, input.p0p1.zw, vNDC.xy, g_vViewport.zw - g_vViewport.xy);
    float3 vPositionWS = NDCToWorld(vNDC, cInvViewProjMatrix);
    float alpha = coverage;

    // Update the alpha to have proper value (accounting for coverage, base alpha, and strand alpha)
    alpha *= strandColor.w;

#ifndef TRESSFX_DEBUG_UAV

    // Early out
    if (alpha < SHORTCUT_MIN_ALPHA)
        return float4(0, 0, 0, 0);

#endif // TRESSFX_DEBUG_UAV

    float3 color = TressFXShading(vPositionWS, vNDC, input.Tangent.xyz, uv, strandUV, strandColor.rgb, input);
    return float4(color * alpha, alpha);
}

[[vk::binding(0, 0)]] Texture2D<float4> HaiColorTexture : register(t0, space0);
[[vk::binding(1, 0)]] Texture2D<float>  AccumInvAlpha : register(t1, space0);

// Fourth pass of ShortCut.
// Full-screen pass that finalizes the weighted average, and blends using the accumulated 1-alpha product.
[earlydepthstencil]
float4 ResolveHairPS(VS_OUTPUT_SCREENQUAD input) : SV_Target
{
    int2 vScreenAddress = int2(input.vPosition.xy);
    
    float fInvAlpha = AccumInvAlpha[vScreenAddress];
    float fAlpha = 1.0 - fInvAlpha;

   if (fAlpha < SHORTCUT_MIN_ALPHA)
        return float4(0, 0, 0, 1);

    float4 fcolor;
    float colorSumX = HaiColorTexture[vScreenAddress].x;
    float colorSumY = HaiColorTexture[vScreenAddress].y;
    float colorSumZ = HaiColorTexture[vScreenAddress].z;
    float colorSumW = HaiColorTexture[vScreenAddress].w;
    fcolor.x = colorSumX / colorSumW;
    fcolor.y = colorSumY / colorSumW;
    fcolor.z = colorSumZ / colorSumW;
    fcolor.xyz *= fAlpha;
    fcolor.w = fInvAlpha;
    return fcolor;
}