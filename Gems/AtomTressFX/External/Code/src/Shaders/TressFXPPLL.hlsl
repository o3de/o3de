//---------------------------------------------------------------------------------------
// Shader code related to per-pixel linked lists.
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

#include "TressFXRendering.hlsl"
#include "TressFXStrands.hlsl"

#define FRAGMENT_LIST_NULL 0xffffffff

[[vk::binding(0, 2)]] cbuffer viewConstants  : register(b0, space2)
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
// Hair Render VS 
PS_INPUT_HAIR RenderHairVS(uint vertexId : SV_VertexID)
{
    TressFXVertex tressfxVert = GetExpandedTressFXVert(vertexId, g_vEye, g_vViewport.zw, g_mVP);

    PS_INPUT_HAIR Output;

    Output.Position = tressfxVert.Position;
    Output.Tangent = tressfxVert.Tangent;
    Output.p0p1 = tressfxVert.p0p1;
    Output.StrandColor = tressfxVert.StrandColor;

    return Output;
}

struct PPLL_STRUCT
{
    uint	depth;
    uint	data;
    uint    color;
    uint    uNext;
};

[[vk::binding(0, 3)]] RWTexture2D<uint> RWFragmentListHead : register(u0, space3);
[[vk::binding(1, 3)]] RWStructuredBuffer<PPLL_STRUCT> LinkedListUAV : register(u1, space3);
[[vk::binding(2, 3)]] RWStructuredBuffer<uint> LinkedListCounter : register(u2, space3);

// Allocate a new fragment location in fragment color, depth, and link buffers
int AllocateFragment(int2 vScreenAddress)
{
    uint newAddress;
    InterlockedAdd(LinkedListCounter[0], 1, newAddress);

    if (newAddress < 0 || newAddress >= NodePoolSize)
        newAddress = FRAGMENT_LIST_NULL;
    return newAddress;
}

// Insert a new fragment at the head of the list. The old list head becomes the
// the second fragment in the list and so on. Return the address of the *old* head.
int MakeFragmentLink(int2 vScreenAddress, int nNewHeadAddress)
{
    int nOldHeadAddress;
    InterlockedExchange(RWFragmentListHead[vScreenAddress], nNewHeadAddress, nOldHeadAddress);
    return nOldHeadAddress;
}

// Write fragment attributes to list location. 
void WriteFragmentAttributes(int nAddress, int nPreviousLink, float4 vData, float3 vColor3, float fDepth)
{
    PPLL_STRUCT element;
    element.data = PackFloat4IntoUint(vData);
    element.color = PackFloat3ByteIntoUint(vColor3, RenderParamsIndex);
    element.depth = asuint(saturate(fDepth));
    element.uNext = nPreviousLink;
    LinkedListUAV[nAddress] = element;
}

//////////////////////////////////////////////////////////////
// PPLL Fill PS
// First pass of PPLL implementation
// Builds up the linked list of hair fragments
[earlydepthstencil]
void PPLLFillPS(PS_INPUT_HAIR input)
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

    // Early out
    if (alpha < 1.f/255.f)
        return;

    // Get the screen address
    int2 vScreenAddress = int2(input.Position.xy);

    // Allocate a new fragment
    int nNewFragmentAddress = AllocateFragment(vScreenAddress);

    int nOldFragmentAddress = MakeFragmentLink(vScreenAddress, nNewFragmentAddress);
    WriteFragmentAttributes(nNewFragmentAddress, nOldFragmentAddress, float4(input.Tangent.xyz * 0.5 + float3(0.5, 0.5, 0.5), alpha), strandColor.xyz, input.Position.z);
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
// Bind data for PPLLResolvePS

#include "TressFXLighting.hlsl"

#define KBUFFER_SIZE 8
#define MAX_FRAGMENTS 512

[[vk::binding(0, 0)]] Texture2D<uint> FragmentListHead : register(t0, space0);
[[vk::binding(1, 0)]] StructuredBuffer<PPLL_STRUCT> LinkedListNodes : register(t1, space0);

#define NODE_DATA(x) LinkedListNodes[x].data
#define NODE_NEXT(x) LinkedListNodes[x].uNext
#define NODE_DEPTH(x) LinkedListNodes[x].depth
#define NODE_COLOR(x) LinkedListNodes[x].color

#define GET_DEPTH_AT_INDEX(uIndex) kBuffer[uIndex].x
#define GET_DATA_AT_INDEX(uIndex) kBuffer[uIndex].y
#define GET_COLOR_AT_INDEX(uIndex) kBuffer[uIndex].z
#define STORE_DEPTH_AT_INDEX(uIndex, uValue) kBuffer[uIndex].x = uValue
#define STORE_DATA_AT_INDEX( uIndex, uValue) kBuffer[uIndex].y = uValue
#define STORE_COLOR_AT_INDEX( uIndex, uValue ) kBuffer[uIndex].z = uValue

// If you change this, you MUST also change TressFXShadeParams in TressFXConstantBuffers.h and ShadeParams in TressFXShortcut.hlsl
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

[[vk::binding(0, 1)]] cbuffer TressFXShadeParams  : register(b0, space1)
{
    ShadeParams HairParams[AMD_TRESSFX_MAX_HAIR_GROUP_RENDER];
};

float3 TressFXShading(float2 pixelCoord, float depth, float3 vTangentCoverage, float3 baseColor, int shaderParamIndex)
{
    float3 vNDC = ScreenPosToNDC(float3(pixelCoord, depth), g_vViewport);
    float3 vPositionWS = NDCToWorld(vNDC, cInvViewProjMatrix);
    float3 vViewDirWS = g_vEye - vPositionWS;

    // Need to expand the tangent that was compressed to store in the buffer
    float3 vTangent = vTangentCoverage.xyz * 2.f - 1.f;

    HairShadeParams params;
    params.Color = baseColor;
    params.HairShadowAlpha = HairParams[shaderParamIndex].ShadowAlpha;
    params.FiberRadius = HairParams[shaderParamIndex].FiberRadius;
    params.FiberSpacing = HairParams[shaderParamIndex].FiberSpacing;

    params.Ka = HairParams[shaderParamIndex].MatKValue.x;
    params.Kd = HairParams[shaderParamIndex].MatKValue.y;
    params.Ks1 = HairParams[shaderParamIndex].MatKValue.z;
    params.Ex1 = HairParams[shaderParamIndex].MatKValue.w;
    params.Ks2 = HairParams[shaderParamIndex].HairKs2;
    params.Ex2 = HairParams[shaderParamIndex].HairEx2;

    return AccumulateHairLight(vTangent, vPositionWS, vViewDirWS, params, vNDC);
}

float4 GatherLinkedList(float2 vfScreenAddress)
{
    uint2 vScreenAddress = uint2(vfScreenAddress);
    uint pointer = FragmentListHead[vScreenAddress];

    if (pointer == FRAGMENT_LIST_NULL)
        discard;

    uint4 kBuffer[KBUFFER_SIZE];

    // Init kbuffer to large values
    [unroll]
    for (int t = 0; t < KBUFFER_SIZE; ++t)
    {
        STORE_DEPTH_AT_INDEX(t, asuint(100000.0));
        STORE_DATA_AT_INDEX(t, 0);
    }

    // Get first K elements from the top (top to bottom)
    // And store them in the kbuffer for later
    for (int p = 0; p < KBUFFER_SIZE; ++p)
    {
        if (pointer != FRAGMENT_LIST_NULL)
        {
            STORE_DEPTH_AT_INDEX(p, NODE_DEPTH(pointer));
            STORE_DATA_AT_INDEX(p, NODE_DATA(pointer));
            STORE_COLOR_AT_INDEX(p, NODE_COLOR(pointer));
            pointer = NODE_NEXT(pointer);
        }
    }

    float4 fcolor = float4(0, 0, 0, 1);

    // Go through the remaining layers of hair
    [allow_uav_condition]
    for (int iFragment = 0; iFragment < MAX_FRAGMENTS && pointer != FRAGMENT_LIST_NULL; ++iFragment)
    {
        if (pointer == FRAGMENT_LIST_NULL) break;

        int id = 0;
        float max_depth = 0;

        // Find the current furthest sample in the KBuffer
        for (int i = 0; i < KBUFFER_SIZE; i++)
        {
            float fDepth = asfloat(GET_DEPTH_AT_INDEX(i));
            if (max_depth < fDepth)
            {
                max_depth = fDepth;
                id = i;
            }
        }

        // Fetch the node data
        uint data = NODE_DATA(pointer);
        uint color = NODE_COLOR(pointer);
        uint nodeDepth = NODE_DEPTH(pointer);
        float fNodeDepth = asfloat(nodeDepth);

        // If the node in the linked list is nearer than the furthest one in the local array, exchange the node 
        // in the local array for the one in the linked list.
        if (max_depth > fNodeDepth)
        {
            uint tmp = GET_DEPTH_AT_INDEX(id);
            STORE_DEPTH_AT_INDEX(id, nodeDepth);
            fNodeDepth = asfloat(tmp);

            tmp = GET_DATA_AT_INDEX(id);
            STORE_DATA_AT_INDEX(id, data);
            data = tmp;

            tmp = GET_COLOR_AT_INDEX(id);
            STORE_COLOR_AT_INDEX(id, color);
            color = tmp;
        }

        // Calculate color contribution from whatever sample we are using
        float4 vData = UnpackUintIntoFloat4(data);
        float3 vTangent = vData.xyz;
        float alpha = vData.w;
        uint shadeParamIndex;	// So we know what settings to shade with
        float3 vColor = UnpackUintIntoFloat3Byte(color, shadeParamIndex);
        
        // Shade the bottom hair layers (cheap shading, just uses scalp base color)
        // Just blend in the color for cheap underhairs
        fcolor.xyz = fcolor.xyz * (1.f - alpha) + (vColor * alpha) * alpha;
        fcolor.w *= (1.f - alpha);

        pointer = NODE_NEXT(pointer);
    }

    // Make sure we are blending the correct number of strands (don't blend more than we have)
    float maxAlpha = 0;

    // Blend the top-most entries
    for (int j = 0; j < KBUFFER_SIZE; j++)
    {
        int id = 0;
        float max_depth = 0;

        // find the furthest node in the array
        for (int i = 0; i < KBUFFER_SIZE; i++)
        {
            float fDepth = asfloat(GET_DEPTH_AT_INDEX(i));
            if (max_depth < fDepth)
            {
                max_depth = fDepth;
                id = i;
            }
        }

        // take this node out of the next search
        uint nodeDepth = GET_DEPTH_AT_INDEX(id);
        uint data = GET_DATA_AT_INDEX(id);
        uint color = GET_COLOR_AT_INDEX(id);

        // take this node out of the next search
        STORE_DEPTH_AT_INDEX(id, 0);

        // Use high quality shading for the nearest k fragments
        float fDepth = asfloat(nodeDepth);
        float4 vData = UnpackUintIntoFloat4(data);
        float3 vTangent = vData.xyz;
        float alpha = vData.w;
        uint shadeParamIndex;	// So we know what settings to shade with
        float3 vColor = UnpackUintIntoFloat3Byte(color, shadeParamIndex);
        float3 fragmentColor = TressFXShading(vfScreenAddress, fDepth, vTangent, vColor, shadeParamIndex);

        // Blend in the fragment color
        fcolor.xyz = fcolor.xyz * (1.f - alpha) + (fragmentColor * alpha) * alpha;
        fcolor.w *= (1.f - alpha);
    }

    return fcolor;
}

//////////////////////////////////////////////////////////////
// PPLL Resolve PS
// Full-screen pass that sorts through the written samples and shades the hair
[earlydepthstencil]
float4 PPLLResolvePS(VS_OUTPUT_SCREENQUAD input) : SV_Target
{
    return GatherLinkedList(input.vPosition.xy);
}