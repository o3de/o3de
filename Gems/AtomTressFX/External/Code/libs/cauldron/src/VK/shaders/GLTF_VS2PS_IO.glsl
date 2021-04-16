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
//  For VS layout
//--------------------------------------------------------------------------------------

#ifdef ID_4VS_POSITION
    layout (location = ID_4VS_POSITION) in vec4 a_Position;
#endif

#ifdef ID_4VS_COLOR_0
    layout (location = ID_4VS_COLOR_0) in  vec3 a_Color0;
#endif

#ifdef ID_4VS_TEXCOORD_0
    layout (location = ID_4VS_TEXCOORD_0) in  vec2 a_UV0;
#endif

#ifdef ID_4VS_TEXCOORD_1
    layout (location = ID_4VS_TEXCOORD_1) in  vec2 a_UV1;
#endif

#ifdef ID_4VS_NORMAL
    layout (location = ID_4VS_NORMAL) in  vec3 a_Normal;
#endif

#ifdef ID_4VS_TANGENT
    layout (location = ID_4VS_TANGENT) in vec4 a_Tangent;
#endif

#ifdef ID_4VS_WEIGHTS_0
    layout (location = ID_4VS_WEIGHTS_0) in  vec4 a_Weights0;
#endif

#ifdef ID_4VS_WEIGHTS_1
    layout (location = ID_4VS_WEIGHTS_1) in  vec4 a_Weights1;
#endif

#ifdef ID_4VS_JOINTS_0
    layout (location = ID_4VS_JOINTS_0) in  uvec4 a_Joints0;
#endif

#ifdef ID_4VS_JOINTS_1
    layout (location = ID_4VS_JOINTS_1) in  uvec4 a_Joints1;
#endif

//--------------------------------------------------------------------------------------
//  For PS layout (make sure this layout matches the one in glTF20-frag.glsl)
//--------------------------------------------------------------------------------------

#ifdef ID_4PS_COLOR_0
    layout (location = ID_4PS_COLOR_0) out  vec3 v_Color0;
#endif

#ifdef ID_4PS_TEXCOORD_0
    layout (location = ID_4PS_TEXCOORD_0) out vec2 v_UV0;
#endif

#ifdef ID_4PS_TEXCOORD_1
    layout (location = ID_4PS_TEXCOORD_1) out vec2 v_UV1;
#endif

#ifdef ID_4PS_NORMAL
    layout (location = ID_4PS_NORMAL) out vec3 v_Normal;
#endif

#ifdef ID_4PS_TANGENT
    layout (location = ID_4PS_TANGENT) out vec3 v_Tangent;
    layout (location = ID_4PS_LASTID+1) out vec3 v_Binormal;
#endif

#ifdef ID_4PS_WORLDPOS
layout (location = ID_4PS_WORLDPOS) out vec3 v_WorldPos;
#endif
