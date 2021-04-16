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

#ifdef ID_SKINNING_MATRICES
    layout (std140, binding = ID_SKINNING_MATRICES) uniform perSkeleton
    {
        mat4 u_ModelMatrix[200];
    } myPerSkeleton;

mat4 GetSkinningMatrix(vec4 Weights, uvec4 Joints)
{
    mat4 skinningMatrix =
        Weights.x * myPerSkeleton.u_ModelMatrix[Joints.x] +
        Weights.y * myPerSkeleton.u_ModelMatrix[Joints.y] +
        Weights.z * myPerSkeleton.u_ModelMatrix[Joints.z] +
        Weights.w * myPerSkeleton.u_ModelMatrix[Joints.w];
    return skinningMatrix;
}
#endif
