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
    float u_weight;
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

void main()
{
    outColor =  myPerFrame.u_weight * texture( inputSampler, inTexCoord).rgba;
}
