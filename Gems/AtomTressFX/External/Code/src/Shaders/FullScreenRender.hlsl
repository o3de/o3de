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
// FullScreen PS
[[vk::binding(0, 0)]] Texture2D<float4> ColorTexture : register(t0, space0);

// Second pass of ShortCut.
// Full-screen pass that writes the farthest of the near depths for depth culling.
float4 FullScreenPS(VS_OUTPUT_SCREENQUAD input) : SV_Target0
{
    // Output the color
    return float4( ColorTexture[input.vPosition.xy].xyz, 1.f );
}