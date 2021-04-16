#version 450

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
 
// This shader code was ported from https://github.com/KhronosGroup/glTF-WebGL-PBR
// All credits should go to his original author.

//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf

//#extension GL_OES_standard_derivatives : enable

#extension GL_EXT_shader_texture_lod: enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// this makes the structures declared with a scalar layout match the c structures
#extension GL_EXT_scalar_block_layout : enable

precision highp float;

#define USE_PUNCTUAL

//--------------------------------------------------------------------------------------
//  IO structures
//--------------------------------------------------------------------------------------

//  For PS layout (make sure this layout matches the one in glTF20-vert.glsl)

#ifdef ID_4PS_COLOR_0
    layout (location = ID_4PS_COLOR_0) in  vec4 v_Color0;
#endif

#ifdef ID_4PS_TEXCOORD_0
    layout (location = ID_4PS_TEXCOORD_0) in vec2 v_UV0;
#endif

#ifdef ID_4PS_TEXCOORD_1
    layout (location = ID_4PS_TEXCOORD_1) in vec2 v_UV1;
#endif
    
#ifdef ID_4PS_NORMAL
    layout (location = ID_4PS_NORMAL) in vec3 v_Normal;
#endif

#ifdef ID_4PS_TANGENT
    layout (location = ID_4PS_TANGENT) in vec3 v_Tangent;
    layout (location = ID_4PS_LASTID+1) in vec3 v_Binormal;
#endif

#ifdef ID_4PS_WORLDPOS
layout (location = ID_4PS_WORLDPOS) in vec3 v_WorldPos;
#endif

//  For OM layout

layout (location = 0) out vec4 outColor;


//--------------------------------------------------------------------------------------
//  Remove texture references if the material doesn't have texture coordinates
//--------------------------------------------------------------------------------------

#ifndef ID_4PS_TEXCOORD_0
    #if ID_normalTexCoord == 0
        #undef ID_normalTexCoord
    #endif
    #if ID_emissiveTexCoord == 0
        #undef ID_emissiveTexCoord
    #endif
    #if ID_occlusionTexCoord == 0
        #undef ID_occlusionTexCoord
    #endif
    #if ID_baseTexCoord == 0
        #undef ID_baseTexCoord
    #endif
    #if ID_metallicRoughnessTextCoord == 0
        #undef ID_metallicRoughnessTextCoord
    #endif
#endif

#ifndef ID_4PS_TEXCOORD_1
    #if ID_normalTexCoord == 1
        #undef ID_normalTexCoord
    #endif
    #if ID_emissiveTexCoord == 1
        #undef ID_emissiveTexCoord
    #endif
    #if ID_occlusionTexCoord == 1
        #undef ID_occlusionTexCoord
    #endif
    #if ID_baseTexCoord == 1
        #undef ID_baseTexCoord
    #endif
    #if ID_metallicRoughnessTextCoord == 1
        #undef ID_metallicRoughnessTextCoord
    #endif
#endif

//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Per Frame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual

struct Light
{
    mat4          mLightViewProj;

    vec3          direction;
    float         range;

    vec3          color;
    float         intensity;
    
    vec3          position;
    float         innerConeCos;

    float         outerConeCos;
    int           type;
    float         depthBias;
    int           shadowMapIndex;    
};

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

layout (scalar, set=0, binding = 0) uniform perFrame 
{
    mat4          u_CameraViewProj;
	mat4          u_mCameraViewProjInverse;
    vec4          u_CameraPos;
    float         u_iblFactor;
    float         u_EmissiveFactor;

    int           padding;
    int           u_lightCount;    
    Light         u_lights[4];
}myPerFrame;


//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------

layout (scalar, set=0, binding = 1) uniform perObject 
{
    mat4 u_World;
    vec4 u_EmissiveFactor;

    // pbrMetallicRoughness
    vec4 u_BaseColorFactor;
    float u_MetallicFactor;
    float u_RoughnessFactor;
    vec2 padding;

    // KHR_materials_pbrSpecularGlossiness
    vec4 u_DiffuseFactor;
    vec3 u_SpecularFactor;
    float u_GlossinessFactor;
} myPerObject;


#include "textures.glsl"
#include "functions.glsl"
#include "shadowFiltering.h"

struct MaterialInfo
{
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    vec3 reflectance0;            // full reflectance color (normal incidence angle)

    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting

    vec3 reflectance90;           // reflectance color at grazing angle
    vec3 specularColor;           // color contribution from specular lighting
};


// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
vec3 getIBLContribution(MaterialInfo materialInfo, vec3 n, vec3 v)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);
    
    float u_MipCount = 9.0; // resolution of 512x512 of the IBL
    float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
    vec3 reflection = normalize(reflect(-v, n));

    vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

    vec3 diffuseLight = texture(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
    vec3 specularLight = textureLod(u_SpecularEnvSampler, reflection, lod).rgb;
#else
    vec3 specularLight = texture(u_SpecularEnvSampler, reflection).rgb;
#endif

    vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
    vec3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

    return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(MaterialInfo materialInfo)
{
    return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float NdotL = angularInfo.NdotL;
    float NdotV = angularInfo.NdotV;
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
    float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
    return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}

vec3 getPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        vec3 F = specularReflection(materialInfo, angularInfo);
        float Vis = visibilityOcclusion(materialInfo, angularInfo);
        float D = microfacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
        vec3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return vec3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(mix(1,0, distance/ range),0);//max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos < innerConeCos)
        {
            return smoothstep(outerConeCos, innerConeCos, actualCos);
        }
        return 1.0;
    }
    return 0.0;
}

vec3 applyDirectionalLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    vec3 pointToLight = light.direction;
    vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return light.intensity * light.color * shade;
}

vec3 applyPointLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    vec3 pointToLight = light.position - v_WorldPos;
    float distance = length(pointToLight);
    float attenuation = getRangeAttenuation(light.range, distance);
    vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    vec3 pointToLight = light.position - v_WorldPos;
    float distance = length(pointToLight);
    float rangeAttenuation = getRangeAttenuation(light.range, distance);
    float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
    vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

//#define DEBUG_OUTPUT
//#define DEBUG_METALLIC
//#define DEBUG_ROUGHNESS 
//#define DEBUG_BASECOLOR
//#define DEBUG_F0
//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------
void main()
{
    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    float perceptualRoughness = 0.0;
    float metallic = 0.0;
    vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 diffuseColor = vec3(0.0);
    vec3 specularColor= vec3(0.0);
    vec3 f0 = vec3(0.04);

#ifdef MATERIAL_SPECULARGLOSSINESS

#ifdef ID_specularGlossinessTexture
    vec4 sgSample = (texture(u_specularGlossinessSampler, getSpecularGlossinessUV()));
    perceptualRoughness = (1.0 - sgSample.a * myPerObject.u_GlossinessFactor); // glossiness to roughness
    f0 = sgSample.rgb * myPerObject.u_SpecularFactor; // specular
#else
    f0 = myPerObject.u_SpecularFactor;
    perceptualRoughness = 1.0 - myPerObject.u_GlossinessFactor;
#endif // ! HAS_SPECULAR_GLOSSINESS_MAP

#ifdef ID_diffuseTexture
    baseColor = texture(u_diffuseSampler, getDiffuseUV()) * myPerObject.u_DiffuseFactor;
#else
    baseColor = myPerObject.u_DiffuseFactor;
#endif // !HAS_DIFFUSE_MAP

    baseColor *= getVertexColor();

    // f0 = specular
    specularColor = f0;
    float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
    diffuseColor = baseColor.rgb * oneMinusSpecularStrength;

#ifdef DEBUG_METALLIC
    // do conversion between metallic M-R and S-G metallic
    metallic = solveMetallic(baseColor.rgb, specularColor, oneMinusSpecularStrength);
#endif // ! DEBUG_METALLIC

#endif // ! MATERIAL_SPECULARGLOSSINESS

#ifdef MATERIAL_METALLICROUGHNESS

#ifdef ID_metallicRoughnessTexture
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    vec4 mrSample = texture(u_MetallicRoughnessSampler, getMetallicRoughnessUV());
    perceptualRoughness = mrSample.g * myPerObject.u_RoughnessFactor;
    metallic = mrSample.b * myPerObject.u_MetallicFactor;
#else
    metallic = myPerObject.u_MetallicFactor;
    perceptualRoughness = myPerObject.u_RoughnessFactor;
#endif

    // The albedo may be defined from a base texture or a flat color
#if defined(ID_baseColorTexture) && defined(ID_baseTexCoord)
    baseColor = (texture(u_BaseColorSampler, getBaseColorUV())) * myPerObject.u_BaseColorFactor;
#else
    baseColor = myPerObject.u_BaseColorFactor;
#endif

    baseColor *= getVertexColor();

    diffuseColor = baseColor.rgb * (vec3(1.0) - f0) * (1.0 - metallic);

    specularColor = mix(f0, baseColor.rgb, metallic);

#endif // ! MATERIAL_METALLICROUGHNESS

	discardPixelIfAlphaCutOff();

#ifdef MATERIAL_UNLIT
    outColor = vec4((baseColor.rgb), baseColor.a);
    return;
#endif

    perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    vec3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

    MaterialInfo materialInfo = MaterialInfo(
        perceptualRoughness,
        specularEnvironmentR0,
        alphaRoughness,
        diffuseColor,
        specularEnvironmentR90,
        specularColor
    );

    // LIGHTING

    vec3 color = vec3(0.0, 0.0, 0.0);
    vec3 normal = getNormal();
    vec3 view = normalize(myPerFrame.u_CameraPos.xyz - v_WorldPos);

#if (DEF_doubleSided == 1)
    if (dot(normal, view) < 0)
    {
        normal = -normal;
    }
#endif

#ifdef USE_PUNCTUAL
    for (int i = 0; i < myPerFrame.u_lightCount; ++i)
    {
        Light light = myPerFrame.u_lights[i];
        if (light.type == LightType_Directional)
        {
            color += applyDirectionalLight(light, materialInfo, normal, view);
        }
        else if (light.type == LightType_Point)
        {            
            color += applyPointLight(light, materialInfo, normal, view);
        }
        else if (light.type == LightType_Spot)
        {            
            float shadowFactor = DoSpotShadow(v_WorldPos, light);
            color += applySpotLight(light, materialInfo, normal, view) * shadowFactor;
        }
    }
#endif

    // Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
    color += getIBLContribution(materialInfo, normal, view) * myPerFrame.u_iblFactor;
#endif

    float ao = 1.0;
    // Apply optional PBR terms for additional (optional) shading
#ifdef ID_occlusionTexture
    ao = texture(u_OcclusionSampler,  getOcclusionUV()).r;
    color = color * ao; //mix(color, color * ao, myPerFrame.u_OcclusionStrength);
#endif

    vec3 emissive = vec3(0);
#ifdef ID_emissiveTexture
    emissive = (texture(u_EmissiveSampler, getEmissiveUV())).rgb * myPerObject.u_EmissiveFactor.rgb * myPerFrame.u_EmissiveFactor;
    color += emissive;
#endif

#ifndef DEBUG_OUTPUT // no debug

   // regular shading
   outColor = vec4(color, baseColor.a);
    
#else // debug output

    #ifdef DEBUG_METALLIC
        outColor.rgb = vec3(metallic);
    #endif

    #ifdef DEBUG_ROUGHNESS
        outColor.rgb = vec3(perceptualRoughness);
    #endif

    #ifdef DEBUG_NORMAL
        #ifdef ID_normalTexCoord
            outColor.rgb = texture(u_NormalSampler, getNormalUV()).rgb;
        #else
            outColor.rgb = vec3(0.5, 0.5, 1.0);
        #endif
    #endif

    #ifdef DEBUG_BASECOLOR
        outColor.rgb = (baseColor.rgb);
    #endif

    #ifdef DEBUG_OCCLUSION
        outColor.rgb = vec3(ao);
    #endif

    #ifdef DEBUG_EMISSIVE
        outColor.rgb = (emissive);
    #endif

    #ifdef DEBUG_F0
        outColor.rgb = vec3(f0);
    #endif

    #ifdef DEBUG_ALPHA
        outColor.rgb = vec3(baseColor.a);
    #endif

    outColor.a = 1.0;

#endif // !DEBUG_OUTPUT
}