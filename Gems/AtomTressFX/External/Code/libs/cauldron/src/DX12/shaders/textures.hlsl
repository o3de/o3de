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
//  Texture and samplers bindings
//
//--------------------------------------------------------------------------------------
//#define CONCAT(a,b) a ## b
//#define TEX(b) CONCAT(t,b)
//#define SMP(b) CONCAT(s,b)
#define TEXCOORD(id) CONCAT(Input.UV, id)

#ifdef ID_baseColorTexture
Texture2D        baseColorTexture              :register(TEX(ID_baseColorTexture));
SamplerState     samBaseColor                  :register(SMP(ID_baseColorTexture));
#endif      

#ifdef ID_normalTexture
Texture2D        normalTexture                 :register(TEX(ID_normalTexture));
SamplerState     samNormal                     :register(SMP(ID_normalTexture));
#endif      

#ifdef ID_emissiveTexture  
Texture2D        emissiveTexture               :register(TEX(ID_emissiveTexture));
SamplerState     samEmissive                   :register(SMP(ID_emissiveTexture));
#endif 

#ifdef ID_metallicRoughnessTexture
Texture2D        metallicRoughnessTexture      :register(TEX(ID_metallicRoughnessTexture));
SamplerState     samMetallicRoughness          :register(SMP(ID_metallicRoughnessTexture));
#endif      

#if defined(ID_occlusionTexture) 
Texture2D        occlusionTexture          :register(TEX(ID_occlusionTexture));
SamplerState     samOcclusion              :register(SMP(ID_occlusionTexture));
static const float u_OcclusionStrength = 1.0;
#endif 

#ifdef ID_diffuseCube
TextureCube      diffuseCube                   :register(TEX(ID_diffuseCube));
SamplerState     samDiffuseCube                :register(SMP(ID_diffuseCube));
#endif 

#ifdef ID_specularCube
TextureCube      specularCube                  :register(TEX(ID_specularCube));
SamplerState     samSpecularCube               :register(SMP(ID_specularCube));
#define USE_TEX_LOD
#endif 

#ifdef ID_brdfTexture
Texture2D        brdfTexture                   :register(TEX(ID_brdfTexture));
SamplerState     samBRDF                       :register(SMP(ID_brdfTexture));
#endif 

// for the specularGlossinessTexture pbr model

#ifdef ID_diffuseTexture
Texture2D        diffuseTexture                :register(TEX(ID_diffuseTexture));
SamplerState     samDiffuse                    :register(SMP(ID_diffuseTexture));
#endif

#ifdef ID_specularGlossinessTexture
Texture2D        specularGlossinessTexture     :register(TEX(ID_specularGlossinessTexture));
SamplerState     samSpecularGlossiness         :register(SMP(ID_specularGlossinessTexture));
#endif


float2 getNormalUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0 , 1.0);
#ifdef ID_normalTexCoord
    uv.xy = TEXCOORD(ID_normalTexCoord);
    #ifdef HAS_NORMAL_UV_TRANSFORM
    uv *= u_NormalUVTransform;
    #endif
#endif
    return uv.xy;
}

float2 getEmissiveUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_emissiveTexCoord
    uv.xy = TEXCOORD(ID_emissiveTexCoord);
    #ifdef HAS_EMISSIVE_UV_TRANSFORM
    uv *= u_EmissiveUVTransform;
    #endif
#endif

    return uv.xy;
}

float2 getOcclusionUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_occlusionTexCoord
    uv.xy = TEXCOORD(ID_occlusionTexCoord);
    #ifdef HAS_OCCLSION_UV_TRANSFORM
    uv *= u_OcclusionUVTransform;
    #endif
#endif
    return uv.xy;
}

float2 getBaseColorUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_baseTexCoord
    uv.xy = TEXCOORD(ID_baseTexCoord);
    #ifdef HAS_BASECOLOR_UV_TRANSFORM
    uv *= u_BaseColorUVTransform;
    #endif
#endif
    return uv.xy;
}

float2 getMetallicRoughnessUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_metallicRoughnessTextCoord
    uv.xy = TEXCOORD(ID_metallicRoughnessTextCoord);
    #ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
    uv *= u_MetallicRoughnessUVTransform;
    #endif
#endif
    return uv.xy;	
}

float2 getSpecularGlossinessUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_specularGlossinessTextCoord
    uv.xy = TEXCOORD(ID_specularGlossinessTextCoord);
#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
    uv *= u_SpecularGlossinessUVTransform;
#endif
#endif
    return uv.xy;
}

float2 getDiffuseUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_diffuseTextCoord
    uv.xy = TEXCOORD(ID_diffuseTextCoord);
#ifdef HAS_DIFFUSE_UV_TRANSFORM
    uv *= u_DiffuseUVTransform;
#endif
#endif
    return uv.xy;
}

float4 getBaseColorTexture(VS_OUTPUT_SCENE Input)
{
#ifdef ID_baseColorTexture
    return baseColorTexture.Sample(samBaseColor, getBaseColorUV(Input));
#else 
    return float4(0, 0, 0, 1); //OPAQUE
#endif
}

float3 getNormalTexture(VS_OUTPUT_SCENE Input)
{
#ifdef ID_normalTexture
    return normalTexture.Sample(samNormal, getNormalUV(Input)).rgb;
#else 
    return float3(0, 0, 0); //OPAQUE
#endif
}

void discardPixelIfAlphaCutOff(VS_OUTPUT_SCENE Input)
{
#ifdef ID_baseColorTexture
    float4 baseColor = getBaseColorTexture(Input);

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