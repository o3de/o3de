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
// 
 
//--------------------------------------------------------------------------------------
//  For VS input struct
//--------------------------------------------------------------------------------------

struct VS_INPUT_SCENE
{
    float3 Position     :    POSITION;      // vertex position
#ifdef HAS_NORMAL
    float3 Normal       :    NORMAL;        // this normal comes in per-vertex
#endif    
#ifdef HAS_TANGENT
    float4 Tangent      :    TANGENT;       // this normal comes in per-vertex
#endif    
#ifdef HAS_TEXCOORD_0
    float2 UV0          :    TEXCOORD0;    // vertex texture coords
#endif
#ifdef HAS_TEXCOORD_1
    float2 UV1          :    TEXCOORD1;    // vertex texture coords
#endif

#ifdef HAS_COLOR_0
    float4 Color0       :    COLOR0;
#endif

    // joints weights
    //
#ifdef HAS_WEIGHTS_0
    float4 Weights0     :    WEIGHTS0;
#endif
#ifdef HAS_WEIGHTS_1
    float4 Weights1     :    WEIGHTS1;
#endif

    // joints indices
    //
#ifdef HAS_JOINTS_0
    uint4 Joints0       :    JOINTS0;
#endif
#ifdef HAS_JOINTS_1
    uint4 Joints1       :    JOINTS1;
#endif
};

//--------------------------------------------------------------------------------------
//  For PS input struct
//--------------------------------------------------------------------------------------

struct VS_OUTPUT_SCENE
{
    float4 svPosition   :    SV_POSITION;   // vertex position
    float3 WorldPos     :    WORLDPOS;      // vertex position
#ifdef HAS_NORMAL
    float3 Normal       :    NORMAL;        // this normal comes in per-vertex
#endif        
#ifdef HAS_TANGENT    
    float3 Tangent      :    TANGENT;       // this normal comes in per-vertex
    float3 Binormal     :    BINORMAL;     // this normal comes in per-vertex
#endif        
#ifdef HAS_COLOR_0
    float4 Color0       :    COLOR0;
#endif
#ifdef HAS_TEXCOORD_0
    float2 UV0       :    TEXCOORD0;    // vertex texture coords
#endif
#ifdef HAS_TEXCOORD_1
    float2 UV1       :    TEXCOORD1;    // vertex texture coords
#endif

#ifdef HAS_MOTION_VECTORS
    float4 svCurrPosition : TEXCOORD2; // current's frame vertex position 
    float4 svPrevPosition : TEXCOORD3; // previous' frame vertex position
#endif
};

