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
//
#include "common.h"

#define USE_PUNCTUAL

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------
#include "GLTFPbrPass-IO.hlsl"

//--------------------------------------------------------------------------------------
//  Remove texture references if the material doesn't have texture coordinates
//--------------------------------------------------------------------------------------

#ifndef HAS_TEXCOORD_0
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

#ifndef HAS_TEXCOORD_1
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
// PerFrame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

#include "perFrameStruct.h"

cbuffer cbPerFrame : register(b0)
{
    PerFrame myPerFrame;
};

//--------------------------------------------------------------------------------------
// PerObject structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------

cbuffer cbPerObject : register(b1)
{
    matrix        myPerObject_u_mWorld;
    float4        myPerObject_u_EmissiveFactor;
    
    // pbrMetallicRoughness
    float4        myPerObject_u_BaseColorFactor;
    float         myPerObject_u_MetallicFactor;
    float         myPerObject_u_RoughnessFactor;
    
    float2        myPerObject_u_padding;

    // KHR_materials_pbrSpecularGlossiness
    float4        myPerObject_u_DiffuseFactor;
    float3        myPerObject_u_SpecularFactor;
    float         myPerObject_u_GlossinessFactor;

};

#include "textures.hlsl"
#include "functions.hlsl"
#include "PixelParams.hlsl"
#include "shadowFiltering.h"

struct MaterialInfo
{
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float3 reflectance0;            // full reflectance color (normal incidence angle)

    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    float3 diffuseColor;            // color contribution from diffuse lighting

    float3 reflectance90;           // reflectance color at grazing angle
    float3 specularColor;           // color contribution from specular lighting
};

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
float3 getIBLContribution(MaterialInfo materialInfo, float3 n, float3 v)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);

    float u_MipCount = 9.0; // resolution of 512x512 of the IBL
    float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
    float3 reflection = normalize(reflect(-v, n));

    float2 brdfSamplePoint = clamp(float2(NdotV, materialInfo.perceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    float2 brdf = brdfTexture.Sample(samBRDF, brdfSamplePoint).rg;

    float3 diffuseLight = diffuseCube.Sample(samDiffuseCube, n).rgb;

#ifdef USE_TEX_LOD
    float3 specularLight = specularCube.SampleLevel(samSpecularCube, reflection, lod).rgb;
#else
    float3 specularLight = specularCube.Sample(samSpecularCube, reflection).rgb;
#endif

    float3 diffuse = diffuseLight * materialInfo.diffuseColor;
    float3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

    return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
float3 diffuse(MaterialInfo materialInfo)
{
    return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
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

float3 getPointShade(float3 pointToLight, MaterialInfo materialInfo, float3 normal, float3 view)
{
    AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        float3 F = specularReflection(materialInfo, angularInfo);
        float Vis = visibilityOcclusion(materialInfo, angularInfo);
        float D = microfacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        float3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
        float3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return float3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(lerp(1, 0, distance / range), 0);
    //return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(float3 pointToLight, float3 spotDirection, float outerConeCos, float innerConeCos)
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

float3 applyDirectionalLight(VS_OUTPUT_SCENE Input, Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.direction;
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return light.intensity * light.color * shade;
}

float3 applyPointLight(VS_OUTPUT_SCENE Input, Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.position - Input.WorldPos;
    float distance = length(pointToLight);
    float attenuation = getRangeAttenuation(light.range, distance);
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

float3 applySpotLight(VS_OUTPUT_SCENE Input, Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.position - Input.WorldPos;
    float distance = length(pointToLight);
    float rangeAttenuation = getRangeAttenuation(light.range, distance);
    float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------
float4 mainPS(VS_OUTPUT_SCENE Input) : SV_Target
{
    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    float perceptualRoughness = 0.0;
    float metallic = 0.0;
    float4 baseColor = float4(0.0, 0.0, 0.0, 1.0);
    float3 diffuseColor = float3(0.0, 0.0, 0.0);
    float3 specularColor = float3(0.0, 0.0, 0.0);
    float3 f0 = float3(0.04, 0.04, 0.04);

#ifdef MATERIAL_SPECULARGLOSSINESS

#ifdef ID_specularGlossinessTexture
    float4 sgSample = specularGlossinessTexture.Sample(samSpecularGlossiness, getSpecularGlossinessUV(Input));
    perceptualRoughness = (1.0 - sgSample.a * myPerObject_u_GlossinessFactor); // glossiness to roughness
    f0 = sgSample.rgb * myPerObject_u_SpecularFactor; // specular
#else
    f0 = myPerObject_u_SpecularFactor;
    perceptualRoughness = 1.0 - myPerObject_u_GlossinessFactor;
#endif // ! HAS_SPECULAR_GLOSSINESS_MAP

#ifdef ID_diffuseTexture
    baseColor = (diffuseTexture.Sample(samDiffuse, getDiffuseUV(Input))) * myPerObject_u_DiffuseFactor;
#else
    baseColor = myPerObject_u_DiffuseFactor;
#endif // !HAS_DIFFUSE_MAP

    baseColor *= getPixelColor(Input);

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
    float4 mrSample = metallicRoughnessTexture.Sample(samMetallicRoughness, getMetallicRoughnessUV(Input));
    perceptualRoughness = mrSample.g * myPerObject_u_RoughnessFactor;
    metallic = mrSample.b * myPerObject_u_MetallicFactor;
#else
    metallic = myPerObject_u_MetallicFactor;
    perceptualRoughness = myPerObject_u_RoughnessFactor;
#endif

    // The albedo may be defined from a base texture or a flat color
#ifdef ID_baseColorTexture
    baseColor = getBaseColorTexture(Input) * myPerObject_u_BaseColorFactor;
#else
    baseColor = myPerObject_u_BaseColorFactor;
#endif

    baseColor *= getPixelColor(Input);

    diffuseColor = baseColor.rgb * (float3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);

    specularColor = lerp(f0, baseColor.rgb, metallic);

#endif // ! MATERIAL_METALLICROUGHNESS

    discardPixelIfAlphaCutOff(Input);

#ifdef MATERIAL_UNLIT
    outColor = float4((baseColor.rgb), baseColor.a);
    return outColor;
#endif

    perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    float3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    float3 specularEnvironmentR90 = float3(1.0, 1.0, 1.0)*clamp(reflectance * 50.0, 0.0, 1.0);

    MaterialInfo materialInfo = {
        perceptualRoughness,
        specularEnvironmentR0,
        alphaRoughness,
        diffuseColor,
        specularEnvironmentR90,
        specularColor
    };

    // LIGHTING

    float3 color = float3(0.0, 0.0, 0.0);
    float3 normal = getPixelNormal(Input);
    float3 view = normalize(myPerFrame.u_CameraPos.xyz - Input.WorldPos);

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
        float shadowFactor = CalcShadows(Input.WorldPos.xyz, int2(Input.svPosition.xy), light);
        if (light.type == LightType_Directional)
        {
            color += applyDirectionalLight(Input, light, materialInfo, normal, view) * shadowFactor;
        }
        else if (light.type == LightType_Point)
        {
            color += applyPointLight(Input, light, materialInfo, normal, view) * shadowFactor;
        }
        else if (light.type == LightType_Spot)
        {
            color += applySpotLight(Input, light, materialInfo, normal, view) * shadowFactor;
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
    ao = occlusionTexture.Sample(samOcclusion, getOcclusionUV(Input)).r;
    color = color * ao; //mix(color, color * ao, myPerFrame.u_OcclusionStrength);
#endif

    float3 emissive = float3(0,0,0);
#ifdef ID_emissiveTexture
    emissive = (emissiveTexture.Sample(samEmissive, getEmissiveUV(Input))).rgb * myPerObject_u_EmissiveFactor.rgb * myPerFrame.u_EmissiveFactor;
    color += emissive;
#endif

#ifndef DEBUG_OUTPUT // no debug

    // regular shading
    float4 outColor = float4(color, baseColor.a);

#else // debug output

#ifdef DEBUG_METALLIC
    outColor.rgb = float3(metallic);
#endif

#ifdef DEBUG_ROUGHNESS
    outColor.rgb = float3(perceptualRoughness);
#endif

#ifdef DEBUG_NORMAL
#ifdef ID_normalTexture
    outColor.rgb = texture(u_NormalSampler, getNormalUV(Input)).rgb;
#else
    outColor.rgb = float3(0.5, 0.5, 1.0);
#endif
#endif

#ifdef DEBUG_BASECOLOR
    outColor.rgb = (baseColor.rgb);
#endif

#ifdef DEBUG_OCCLUSION
    outColor.rgb = float3(ao);
#endif

#ifdef DEBUG_EMISSIVE
    outColor.rgb = (emissive);
#endif

#ifdef DEBUG_F0
    outColor.rgb = float3(f0);
#endif

#ifdef DEBUG_ALPHA
    outColor.rgb = float3(baseColor.a);
#endif

    outColor.a = 1.0;

#endif // !DEBUG_OUTPUT

    return outColor;
}