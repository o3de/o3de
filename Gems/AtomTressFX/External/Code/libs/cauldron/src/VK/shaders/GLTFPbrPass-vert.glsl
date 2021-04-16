#version 450

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

//#extension GL_ARB_separate_shader_objects : enable
//#extension GL_ARB_shading_language_420pack : enable

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.glsl"

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------

layout (std140, binding = ID_PER_FRAME) uniform perFrame
{
    mat4 u_MVPMatrix;
} myPerFrame;

layout (std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4 u_ModelMatrix;
} myPerObject;


mat4 GetWorldMatrix()
{
    return myPerObject.u_ModelMatrix;
}

mat4 GetCameraViewProj()
{
    return myPerFrame.u_MVPMatrix;
}

#include "GLTFVertexFactory.glsl"

//--------------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------------
void main()
{
	gltfVertexFactory();
}

