// AMD Cauldron code
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

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
    float2 u_invSize;
    int u_mipLevel;
}

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
struct VERTEX
{
    float2 vTexcoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D        inputTex         :register(t0);
SamplerState     samLinearMirror  :register(s0);

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

static float2 offsets[9] = { 
    float2( 1, 1), float2( 0, 1), float2(-1, 1), 
    float2( 1, 0), float2( 0, 0), float2(-1, 0), 
    float2( 1,-1), float2( 0,-1), float2(-1,-1)
    };

float4 mainPS(VERTEX Input) : SV_Target
{
    // gaussian like downsampling
    
    float4 color = float4(0,0,0,0);

    if (u_mipLevel==0)
    {
        for(int i=0;i<9;i++)
            color += log(max(inputTex.Sample(samLinearMirror, Input.vTexcoord + (2 * u_invSize * offsets[i])), float4(0.01, 0.01, 0.01, 0.01) ));
        return exp(color / 9.0f);
    }
    else
    {
        for(int i=0;i<9;i++)
            color += inputTex.Sample(samLinearMirror, Input.vTexcoord + (2 * u_invSize * offsets[i]) );
        return color / 9.0f;
    }
}
