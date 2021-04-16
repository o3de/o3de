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
[[vk::binding(0)]] cbuffer cbPerFrame : register(b0)
{
    matrix u_mClipToWord;
}

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
struct VERTEX
{
    [[vk::location(0)]] float2 vTexcoord : TEXCOORD; 
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
[[vk::binding(1)]] TextureCube      inputTex         :register(t0);
[[vk::binding(1)]] SamplerState     samLinearWrap    :register(s0);

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
[[vk::location(0)]] float4 main(VERTEX Input) : SV_Target 
{
    float4 clip = float4(2 * Input.vTexcoord.x - 1, 1 - 2 * Input.vTexcoord.y, 1, 1);

    float4 pixelDir = mul(u_mClipToWord, clip);

    return inputTex.Sample(samLinearWrap, pixelDir.xyz)*.02;
}
