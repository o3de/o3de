#version 400

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

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
layout (std140, binding = 0) uniform perFrame
{
    vec2 u_invSize;
    int u_mipLevel;
} myPerFrame;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
layout(set=0, binding=1) uniform sampler2D inputSampler;

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

vec2 offsets[9] = { 
    vec2( 1, 1), vec2( 0, 1), vec2(-1, 1), 
    vec2( 1, 0), vec2( 0, 0), vec2(-1, 0), 
    vec2( 1,-1), vec2( 0,-1), vec2(-1,-1)
    };

void main()
{
    // gaussian like downsampling

    vec4 color = vec4(0,0,0,0);

    if (myPerFrame.u_mipLevel==0)
    {
        for(int i=0;i<9;i++)
            color += log(max(.01+texture(inputSampler, inTexCoord + (2 * myPerFrame.u_invSize * offsets[i]) ), vec4(0.01, 0.01, 0.01, 0.01) ));
        outColor = exp(color / 9.0f);
    }
    else
    {
        for(int i=0;i<9;i++)
            color += texture(inputSampler, inTexCoord + (2 * myPerFrame.u_invSize * offsets[i]) );
        outColor = color / 9.0f;
    }
}

