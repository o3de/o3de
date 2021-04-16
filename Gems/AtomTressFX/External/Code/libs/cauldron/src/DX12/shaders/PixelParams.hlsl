// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// textures.glsl needs to be included

float4 getPixelColor(VS_OUTPUT_SCENE Input)
{
   float4 color = float4(1.0, 1.0, 1.0, 1.0);
   
#ifdef HAS_COLOR_0
    color = Input.Color0;
#endif

   return color;
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
float3 getPixelNormal(VS_OUTPUT_SCENE Input)
{    
    float2 UV = getNormalUV(Input);

    // Retrieve the tangent space matrix
#ifndef HAS_TANGENT
    float3 pos_dx = ddx(Input.WorldPos);
    float3 pos_dy = ddy(Input.WorldPos);
    float3 tex_dx = ddx(float3(UV, 0.0));
    float3 tex_dy = ddy(float3(UV, 0.0));
    float3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);
    
#ifdef HAS_NORMAL
    float3 ng = normalize(Input.Normal);
#else
    float3 ng = cross(pos_dx, pos_dy);
#endif

    t = normalize(t - ng * dot(ng, t));
    float3 b = normalize(cross(ng, t));
    float3x3 tbn = float3x3(t, b, ng);
#else // HAS_TANGENTS
    float3x3 tbn = float3x3(Input.Tangent, Input.Binormal, Input.Normal);
#endif

#ifdef ID_normalTexture
    float3 n = getNormalTexture(Input);
    n = normalize(mul(transpose(tbn),((2.0 * n - 1.0) /* * float3(u_NormalScale, u_NormalScale, 1.0) */)));
#else
    // The tbn matrix is linearly interpolated, so we need to re-normalize
    float3 n = normalize(tbn[2].xyz);
#endif

    return n;
}

