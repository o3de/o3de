//---------------------------------------------------------------------------------------
// Shader code related to lighting and shadowing for TressFX
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

// This is code decoration for our sample.  
// Remove this and the EndHLSL at the end of this file 
// to turn this into valid HLSL

// StartHLSL TressFXRendering
#define AMD_PI 3.1415926
#define AMD_e 2.71828183

#define AMD_TRESSFX_KERNEL_SIZE 5

// We might break this down further.
// If you change this, you MUST also change TressFXRenderParams in TressFXConstantBuffers.h
// Binding (0-1, 0) are in TressFXStrands.hlsl
[[vk::binding(3, 0)]] cbuffer TressFXParameters  : register(b3, space0)
{
    // General information
    float       HairFiberRadius;

    // For deep approximated shadow lookup
    float       ShadowAlpha;
    float       FiberSpacing;

    // For lighting/shading
    float       HairKs2;
    float       HairEx2;
    float3      fPadding0;

    float4      MatKValue;
    
    int         MaxShadowFibers;
    int3        iPadding0;
}

// Separate strand params from pixel render params (so we can index for PPLL)
// If you change this, you MUST also change TressFXStrandParams in TressFXConstantBuffers.h
[[vk::binding(4, 0)]] cbuffer TressFXStrandParameters  : register(b4, space0)
{
    float4      MatBaseColor;
    float4      MatTipColor;

    float       TipPercentage;
    float       StrandUVTilingFactor;
    float       FiberRatio;
    float       FiberRadius;

    int         NumVerticesPerStrand;
    int         EnableThinTip;
    int         NodePoolSize;
    int         RenderParamsIndex;

    // Other params
    int         EnableStrandUV;
    int         EnableStrandTangent;
    int2        iPadding1;
}

// NOTE, we may have to split bindings to TressFXStrands and those to TressFXRendering when implementing PPLL
[[vk::binding(5, 0)]] Texture2D<float4> StrandAlbedoTexture : register(t5, space0);

struct HairShadeParams
{
    float3 Color;
    float HairShadowAlpha;
    float FiberRadius;
    float FiberSpacing;
    float Ka;
    float Kd;
    float Ks1;
    float Ex1;
    float Ks2;
    float Ex2;
};

//--------------------------------------------------------------------------------------
// ComputeCoverage
//
// Calculate the pixel coverage of a hair strand by computing the hair width
//--------------------------------------------------------------------------------------
float ComputeCoverage(float2 p0, float2 p1, float2 pixelLoc, float2 winSize)
{
    // p0, p1, pixelLoc are in d3d clip space (-1 to 1)x(-1 to 1)

    // Scale positions so 1.f = half pixel width
    p0 *= winSize;
    p1 *= winSize;
    pixelLoc *= winSize;

    float p0dist = length(p0 - pixelLoc);
    float p1dist = length(p1 - pixelLoc);
    float hairWidth = length(p0 - p1);

    // will be 1.f if pixel outside hair, 0.f if pixel inside hair
    float outside = any(float2(step(hairWidth, p0dist), step(hairWidth, p1dist)));

    // if outside, set sign to -1, else set sign to 1
    float sign = outside > 0.f ? -1.f : 1.f;

    // signed distance (positive if inside hair, negative if outside hair)
    float relDist = sign * saturate(min(p0dist, p1dist));

    // returns coverage based on the relative distance
    // 0, if completely outside hair edge
    // 1, if completely inside hair edge
    return (relDist + 1.f) * 0.5f;
}

// EndHLSL

