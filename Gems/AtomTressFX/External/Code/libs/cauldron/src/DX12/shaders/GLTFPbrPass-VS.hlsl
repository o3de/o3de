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
#include "common.h"

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------

#include "GLTFPbrPass-IO.hlsl"

//--------------------------------------------------------------------------------------
//  Constant buffers
//--------------------------------------------------------------------------------------

cbuffer cbPerFrame : register(b0)
{
    matrix        myPerFrame_u_mCameraViewProj;
    float4        myPerFrame_u_CameraPos;
    float         myPerFrame_u_iblFactor;
    float         myPerFrame_u_EmissiveFactor;
};

cbuffer cbPerObject : register(b1)
{
    matrix        myPerObject_u_mWorld;
}

matrix GetWorldMatrix()
{
    return myPerObject_u_mWorld;
}

matrix GetCameraViewProj()
{
    return myPerFrame_u_mCameraViewProj;
}

#include "GLTFVertexFactory.hlsl"


//--------------------------------------------------------------------------------------
// mainVS
//--------------------------------------------------------------------------------------
VS_OUTPUT_SCENE mainVS(VS_INPUT_SCENE input)
{
    VS_OUTPUT_SCENE Output = gltfVertexFactory(input);

    return Output;
}

