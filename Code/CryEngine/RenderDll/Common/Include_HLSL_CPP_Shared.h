/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.


#ifdef __cplusplus
// C++
    #pragma once
    #pragma pack(push,4)

    #define hlsl_cbuffer(name)                struct HLSL_##name
    #define hlsl_cbuffer_register(name, reg, slot) \
    enum { SLOT_##name = slot };                   \
    struct HLSL_##name
       
    #define hlsl_int(member)                  int32    member
    #define hlsl_int2(member)                 Vec2i    member
    #define hlsl_int3(member)                 Vec3i    member
    #define hlsl_int4(member)                 Vec4i    member
    #define hlsl_uint(member)                 uint32   member
    #define hlsl_uint2(member)                Vec2ui   member
    #define hlsl_uint3(member)                Vec3ui   member
    #define hlsl_uint4(member)                Vec4ui   member
    #define hlsl_float(member)                float    member
    #define hlsl_float2(member)               Vec2     member
    #define hlsl_float3(member)               Vec3     member
    #define hlsl_float4(member)               Vec4     member
    #define hlsl_matrix44(member)             Matrix44 member
    #define hlsl_matrix34(member)             Matrix34 member
#else //__cplusplus
// HLSL
    #define hlsl_cbuffer(name)                cbuffer  name
    #define hlsl_cbuffer_register(name, reg, float)   cbuffer  name: reg
    #define hlsl_int(member)                  int      member
    #define hlsl_int2(member)                 int2     member
    #define hlsl_int3(member)                 int3     member
    #define hlsl_int4(member)                 int4     member
    #define hlsl_uint(member)                 uint     member
    #define hlsl_uint2(member)                uint2    member
    #define hlsl_uint3(member)                uint3    member
    #define hlsl_uint4(member)                uint4    member
    #define hlsl_float(member)                float    member
    #define hlsl_float2(member)               float2   member
    #define hlsl_float3(member)               float3   member
    #define hlsl_float4(member)               float4   member
    #define hlsl_matrix44(member)             float4x4 member
    #define hlsl_matrix34(member)             float3x4 member
#endif //__cplusplus

// TODO: include in shaders

hlsl_cbuffer(PerPassConstantBuffer_GBuffer)
{
    hlsl_float4(g_VS_WorldViewPos);
    hlsl_matrix44(g_VS_ViewProjMatr);
    hlsl_matrix44(g_VS_ViewProjZeroMatr);
};

#if defined(FEATURE_SVO_GI)
hlsl_cbuffer(PerPassConstantBuffer_Svo)
{
    hlsl_float4(PerPass_SvoTreeSettings0);
    hlsl_float4(PerPass_SvoTreeSettings1);
    hlsl_float4(PerPass_SvoParams0);
    hlsl_float4(PerPass_SvoParams1);
    hlsl_float4(PerPass_SvoParams2);
    hlsl_float4(PerPass_SvoParams3);
    hlsl_float4(PerPass_SvoParams4);
    hlsl_float4(PerPass_SvoParams5);
    hlsl_float4(PerPass_SvoParams6);
};
#endif

hlsl_cbuffer(PerSubPassConstantBuffer_ShadowGen)
{
    hlsl_float4(PerShadow_LightPos);
    hlsl_float4(PerShadow_ViewPos);
    hlsl_float4(PerShadow_FrustumInfo);
    hlsl_float4(PerShadow_BiasInfo);
};

hlsl_cbuffer(PerInstanceConstantBuffer)
{
    hlsl_matrix34(SPIObjWorldMat);
    hlsl_float4(SPIBendInfo);
    hlsl_float4(SPIBendInfoPrev);
    hlsl_float4(SPIAmbientOpacity);
    hlsl_float4(SPIDissolveRef);
};

hlsl_cbuffer(PerViewConstantBuffer)
{
    hlsl_float4(PerView_WorldViewPos);
    hlsl_float4(PerView_WorldViewPosPrev);
    hlsl_matrix44(PerView_ViewProjZeroMatr);
    hlsl_matrix44(PerView_ViewProjZeroMatrPrev);
    hlsl_matrix44(PerView_ViewProjZeroMatrPrevNearest);
    hlsl_float4(PerView_AnimGenParams);

    hlsl_float4(PerView_ViewBasisX);
    hlsl_float4(PerView_ViewBasisY);
    hlsl_float4(PerView_ViewBasisZ);

    hlsl_matrix44(PerView_ViewProjMatr);
    hlsl_matrix44(PerView_ViewProjMatrPrev);
    hlsl_matrix44(PerView_ViewMatr);
    hlsl_matrix44(PerView_ProjMatr);
    hlsl_float4(PerView_TessellationParams);

    hlsl_float4(PerView_ScreenSize);
    hlsl_float4(PerView_HPosScale);
    hlsl_float4(PerView_ProjRatio);
    hlsl_float4(PerView_NearestScaled);
    hlsl_float4(PerView_NearFarClipDist);

    hlsl_float4(PerView_FogColor);

    hlsl_matrix44(PerView_FrustumPlaneEquation);
    hlsl_float4(PerView_JitterParams);
};

hlsl_cbuffer(PerFrameConstantBuffer)
{
    hlsl_float4(PerFrame_VolumetricFogParams);
    hlsl_float4(PerFrame_VolumetricFogRampParams);
    hlsl_float4(PerFrame_VolumetricFogColorGradientBase);
    hlsl_float4(PerFrame_VolumetricFogColorGradientDelta);
    hlsl_float4(PerFrame_VolumetricFogColorGradientParams);
    hlsl_float4(PerFrame_VolumetricFogColorGradientRadial);
    hlsl_float4(PerFrame_VolumetricFogSamplingParams);
    hlsl_float4(PerFrame_VolumetricFogDistributionParams);
    hlsl_float4(PerFrame_VolumetricFogScatteringParams);
    hlsl_float4(PerFrame_VolumetricFogScatteringBlendParams);
    hlsl_float4(PerFrame_VolumetricFogScatteringColor);
    hlsl_float4(PerFrame_VolumetricFogScatteringSecondaryColor);
    hlsl_float4(PerFrame_VolumetricFogHeightDensityParams);
    hlsl_float4(PerFrame_VolumetricFogHeightDensityRampParams);
    hlsl_float4(PerFrame_VolumetricFogDistanceParams);
    hlsl_float4(PerFrame_VolumetricFogGlobalEnvProbe0);
    hlsl_float4(PerFrame_VolumetricFogGlobalEnvProbe1);

    hlsl_float4(PerFrame_SvoLightingParams);

    hlsl_float4(PerFrame_Time);

    hlsl_float4(PerFrame_SunColor);
    hlsl_float4(PerFrame_SunDirection);
    hlsl_float4(PerFrame_HDRParams);

    hlsl_float4(PerFrame_CloudShadingColorSun);
    hlsl_float4(PerFrame_CloudShadingColorSky);
    hlsl_float4(PerFrame_CloudShadowParams);
    hlsl_float4(PerFrame_CloudShadowAnimParams);

    hlsl_float4(PerFrame_CausticsSmoothSunDirection);

    hlsl_float4(PerFrame_DecalZFightingRemedy);
    hlsl_float4(PerFrame_WaterLevel);

    hlsl_float4(PerFrame_StereoParams);
    hlsl_float4(PerFrame_RandomParams);

    hlsl_uint4(PerFrame_MultiLayerAlphaBlendLayerData);
};

#ifdef __cplusplus
// C++
    #pragma pack(pop)

#else //__cplusplus
// HLSL

#endif //__cplusplus
