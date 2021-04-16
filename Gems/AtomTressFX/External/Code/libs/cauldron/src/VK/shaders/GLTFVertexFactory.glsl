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

#include "skinning.h"


void gltfVertexFactory()
{
#ifdef ID_4VS_WEIGHTS_0
    mat4 skinningMatrix;
    skinningMatrix  = GetSkinningMatrix(a_Weights0, a_Joints0);
#ifdef ID_4VS_WEIGHTS_1
    skinningMatrix += GetSkinningMatrix(a_Weights1, a_Joints1);
#endif
#else
    mat4 skinningMatrix =
    {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
#endif

  mat4 transMatrix = GetWorldMatrix() * skinningMatrix;
  vec4 pos = transMatrix * a_Position;

  #ifdef ID_4PS_WORLDPOS
  v_WorldPos = vec3(pos.xyz) / pos.w;
  #endif

  #ifdef ID_4VS_NORMAL
  v_Normal = normalize(vec3(transMatrix * vec4(a_Normal.xyz, 0.0)));

  #ifdef ID_4VS_TANGENT
    v_Tangent = normalize(vec3(transMatrix * vec4(a_Tangent.xyz, 0.0)));
    v_Binormal = cross(v_Normal, v_Tangent) * a_Tangent.w;
  #endif
  #endif

  #ifdef ID_4VS_COLOR_0
    v_Color0 = a_Color0;
  #endif

  #ifdef ID_4VS_TEXCOORD_0
    v_UV0 = a_UV0;
  #endif

  #ifdef ID_4VS_TEXCOORD_1
    v_UV1 = a_UV1;
  #endif

  gl_Position = GetCameraViewProj() * pos; // needs w for proper perspective correction
}

