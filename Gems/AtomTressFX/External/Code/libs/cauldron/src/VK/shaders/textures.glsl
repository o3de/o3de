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

//--------------------------------------------------------------------------------------
//
// Texture and samplers bindings
//
//--------------------------------------------------------------------------------------

#ifdef ID_baseColorTexture
    layout (set=1, binding = ID_baseColorTexture) uniform sampler2D u_BaseColorSampler;
#endif

#ifdef ID_normalTexture
    layout (set=1, binding = ID_normalTexture) uniform sampler2D u_NormalSampler;
#endif

#ifdef ID_emissiveTexture
    layout (set=1, binding = ID_emissiveTexture) uniform sampler2D u_EmissiveSampler;
#endif

#ifdef ID_metallicRoughnessTexture
    layout (set=1, binding = ID_metallicRoughnessTexture) uniform sampler2D u_MetallicRoughnessSampler;
#endif

#ifdef ID_occlusionTexture
    layout (set=1, binding = ID_occlusionTexture) uniform sampler2D u_OcclusionSampler;
    float u_OcclusionStrength  =1.0;
#endif

#ifdef ID_diffuseTexture
    layout (set=1, binding = ID_diffuseTexture) uniform sampler2D u_diffuseSampler;
#endif

#ifdef ID_specularGlossinessTexture
    layout (set=1, binding = ID_specularGlossinessTexture) uniform sampler2D u_specularGlossinessSampler;
#endif

#ifdef USE_IBL
layout (set=1, binding = ID_diffuseCube) uniform samplerCube u_DiffuseEnvSampler;
layout (set=1, binding = ID_specularCube) uniform samplerCube u_SpecularEnvSampler;
#define USE_TEX_LOD
#endif

#ifdef ID_brdfTexture
layout (set=1, binding = ID_brdfTexture) uniform sampler2D u_brdfLUT;
#endif

#define CONCAT(a,b) a ## b
#define TEXCOORD(id) CONCAT(v_UV, id)

vec2 getNormalUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_normalTexCoord
    uv.xy = TEXCOORD(ID_normalTexCoord);
    #ifdef HAS_NORMAL_UV_TRANSFORM
    uv *= u_NormalUVTransform;
    #endif
#endif
    return uv.xy;
}

vec2 getEmissiveUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_emissiveTexCoord
    uv.xy = TEXCOORD(ID_emissiveTexCoord);
    #ifdef HAS_EMISSIVE_UV_TRANSFORM
    uv *= u_EmissiveUVTransform;
    #endif
#endif

    return uv.xy;
}

vec2 getOcclusionUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_occlusionTexCoord
    uv.xy = TEXCOORD(ID_occlusionTexCoord);
    #ifdef HAS_OCCLSION_UV_TRANSFORM
    uv *= u_OcclusionUVTransform;
    #endif
#endif
    return uv.xy;
}

vec2 getBaseColorUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_baseTexCoord
    uv.xy = TEXCOORD(ID_baseTexCoord);
    #ifdef HAS_BASECOLOR_UV_TRANSFORM
    uv *= u_BaseColorUVTransform;
    #endif
#endif
    return uv.xy;
}

vec2 getMetallicRoughnessUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_metallicRoughnessTextCoord
    uv.xy = TEXCOORD(ID_metallicRoughnessTextCoord);
    #ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
    uv *= u_MetallicRoughnessUVTransform;
    #endif
#endif
    return uv.xy;    
}

vec2 getSpecularGlossinessUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_specularGlossinessTexture
    uv.xy = TEXCOORD(ID_specularGlossinessTextCoord);
    #ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
    uv *= u_SpecularGlossinessUVTransform;
    #endif
#endif
    return uv.xy;
}

vec2 getDiffuseUV()
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_diffuseTexture
    uv.xy = TEXCOORD(ID_diffuseTextCoord);
    #ifdef HAS_DIFFUSE_UV_TRANSFORM
    uv *= u_DiffuseUVTransform;
    #endif
#endif
    return uv.xy;
}

vec4 getBaseColorTexture()
{
#ifdef ID_baseColorTexture
    return texture(u_BaseColorSampler, getBaseColorUV());
#else
    return vec4(0, 0, 0, 1); //OPAQUE
#endif
}

vec3 getNormalTexture()
{
#ifdef ID_normalTexture
    return texture(u_NormalSampler, getNormalUV()).rgb;
#else
    return vec3(0, 0, 0); //OPAQUE
#endif
}

void discardPixelIfAlphaCutOff()
{
#ifdef ID_baseColorTexture
    vec4 baseColor = getBaseColorTexture();

#if defined(DEF_alphaMode_BLEND)
        if (baseColor.a == 0)
            discard;
#elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
        if (baseColor.a < DEF_alphaCutoff)
            discard;
#else
        //OPAQUE
#endif
#endif
}