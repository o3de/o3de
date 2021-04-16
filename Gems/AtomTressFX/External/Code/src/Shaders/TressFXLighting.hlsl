//---------------------------------------------------------------------------------------
// Shader code related to lighting and shadowing for TressFX
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// This is code decoration for our sample.  
// Remove this and the EndHLSL at the end of this file 
// to turn this into valid HLSL


// If you change this, you MUST also change TressFXLightParams in TressFXConstantBuffers.h
#define AMD_TRESSFX_MAX_LIGHTS 10

// These come from GltfCommon (just re-using them)
#define LightType_Directional 0
#define LightType_Point 1
#define LightType_Spot 2

struct LightParams
{
                                // nLightShape (ignored) for volumes and stuff ...
                                // nLightIndex (not used in Sushi)
    float   LightIntensity;     // fLightIntensity in Sushi
    float   LightOuterConeCos;  // vLightConeAngles.x in Sushi
    float   LightInnerConeCos;  // vLightConeAngles.y in Sushi
    float   LightRange;         // vLightConeAngles.z in Sushi

    float4  LightPositionWS;    // vLightPosWS in Sushi
    float4  LightDirWS;         // vLightDirWS in Sushi
    float4  LightColor;         // vLightColor in Sushi

    float4x4    ShadowProjection;
    
    float4      ShadowParams;   // Bias, near, far, range 

                                // vScaleWS (not used)
    int     LightType;          // vLightParams.x in Sushi
                                // vLightParams.y is falloff exponent and always 1.f in Sushi
                                // vLightParams.zw are Diffuse and Spec mask in Sushi and are always 1.f
                                // vLightOrientation (not used)
    int     ShadowMapIndex;
    
    int     ShadowMapSize;      // Dimension of the shadow slice we are sampling from
    int     Padding2;
};

[[vk::binding(0, 3)]] cbuffer LightConstants : register(b0, space3)
{
    int			NumLights;
    int			UseDepthApproximation;
    int			Padding4;
    int			Padding5;
    LightParams LightData[AMD_TRESSFX_MAX_LIGHTS];
};

[[vk::binding(1, 3)]] Texture2D<float> ShadowTexture : register(t1, space3);

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float SpotAttenuation(float3 pointToLight, float3 spotDirection, float outerConeCos, float innerConeCos)
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

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float RangeAttenuation(float range, float distance)
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(lerp(1, 0, distance / range), 0);//max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// =============================================================================================================================
// Reduces light to a direction and intensity (taking attenuation and light strength into account)
void GetLightParams(float3 WorldPosition, uint lightIndex, inout float3 LightVector, inout float LightIntensity, HairShadeParams params)
{
    LightIntensity = LightData[lightIndex].LightIntensity;

    // Spot and Directionals use the light direction directly
    LightVector = LightData[lightIndex].LightDirWS;
    if (LightData[lightIndex].LightType == LightType_Directional)
        return;

    // Normalized light vector and squared distance for radial source calculations
    float3 vLightVecWS = LightData[lightIndex].LightPositionWS - WorldPosition;
    float distToLight = length(vLightVecWS);

    // Set the LightVector to actual direction to light
    LightVector = normalize(vLightVecWS);

    // Calculate radial falloff
    LightIntensity *= RangeAttenuation(LightData[lightIndex].LightRange, distToLight);
    
    // Modulate with spot falloff if needed
    if (LightData[lightIndex].LightType == LightType_Spot)
    {
        // Need to inverse the direction going in (as it's direction to light) as Unreal with negate it again under the hood.
        LightIntensity *= SpotAttenuation(LightVector, -LightData[lightIndex].LightDirWS, LightData[lightIndex].LightOuterConeCos, LightData[lightIndex].LightInnerConeCos);
    }
}

// Returns a float3 which is the scale for diffuse, spec term, and colored spec term.
//
// The diffuse term is from Kajiya.
// 
// The spec term is what Marschner calls "R", reflecting directly off the surface of the hair, 
// taking the color of the light like a dielectric specular term.  This highlight is shifted 
// towards the root of the hair.
//
// The colored spec term is caused by light passing through the hair, bouncing off the back, and 
// coming back out.  It therefore picks up the color of the light.  
// Marschner refers to this term as the "TRT" term.  This highlight is shifted towards the 
// tip of the hair.
//
// vEyeDir, vLightDir and vTangentDir are all pointing out.
// coneAngleRadians explained below.
//
// 
// hair has a tiled-conical shape along its lenght.  Sort of like the following.
// 
// \    /
//  \  /
// \    /
//  \  /  
//
// The angle of the cone is the last argument, in radians.  
// It's typically in the range of 5 to 10 degrees
float3 ComputeDiffuseSpecFactors(float3 vEyeDir, float3 vLightDir, float3 vTangentDir, HairShadeParams params, float coneAngleRadians = 10 * AMD_PI / 180)
{
    // in Kajiya's model: diffuse component: sin(t, l)
    float cosTL = (dot(vTangentDir, vLightDir));
    float sinTL = sqrt(1 - cosTL * cosTL);
    float diffuse = sinTL; // here sinTL is apparently larger than 0

    float cosTRL = -cosTL;
    float sinTRL = sinTL;
    float cosTE = (dot(vTangentDir, vEyeDir));
    float sinTE = sqrt(1 - cosTE * cosTE);

    // primary highlight: reflected direction shift towards root (2 * coneAngleRadians)
    float cosTRL_root = cosTRL * cos(2 * coneAngleRadians) - sinTRL * sin(2 * coneAngleRadians);
    float sinTRL_root = sqrt(1 - cosTRL_root * cosTRL_root);
    float specular_root = max(0, cosTRL_root * cosTE + sinTRL_root * sinTE);

    // secondary highlight: reflected direction shifted toward tip (3*coneAngleRadians)
    float cosTRL_tip = cosTRL * cos(-3 * coneAngleRadians) - sinTRL * sin(-3 * coneAngleRadians);
    float sinTRL_tip = sqrt(1 - cosTRL_tip * cosTRL_tip);
    float specular_tip = max(0, cosTRL_tip * cosTE + sinTRL_tip * sinTE);

    return float3(params.Kd * diffuse, params.Ks1 * pow(specular_root, params.Ex1), params.Ks2 * pow(specular_tip, params.Ex2));
}

float LinearizeDepth(float depthNDC, float fNear, float fFar)
{
    return fNear * fFar / (fFar - depthNDC * (fFar - fNear));
}

// fDepthDistanceWS is the world space distance between the point on the surface and the point in the shadow map.
// fFiberSpacing is the assumed, average, world space distance between hair fibers.
// fFiberRadius in the assumed average hair radius.
// fHairAlpha is the alpha value for the hair (in terms of translucency, not coverage.)
// Output is a number between 0 (totally shadowed) and 1 (lets everything through)
float ComputeShadowAttenuation(float fDepthDistanceWS, float fFiberSpacing, float fFiberRadius, float fHairAlpha)
{
    float numFibers = fDepthDistanceWS / (fFiberSpacing * fFiberRadius);	// fiberSpacing + fiberRadius is total distance from 1 fiber to another

    // if occluded by hair, there is at least one fiber
    [flatten] if (fDepthDistanceWS > 1e-5)
        numFibers = max(numFibers, 1);

    return pow(abs(1 - fHairAlpha), numFibers);
}

float ComputeLightShadow(int lightIndex, float3 vPositionWS, in HairShadeParams params)
{
    if (LightData[lightIndex].ShadowMapIndex < 0)
        return 1.f;
    
    float4 shadowTexCoord = MatrixMult( LightData[lightIndex].ShadowProjection, float4(vPositionWS, 1.f) );
    shadowTexCoord.xyz /= shadowTexCoord.w;

    // remember we are splitting the shadow map in 4 quarters 
    // and at this point, everything's in -1 to 1
    // So bring back to 0 -> 0.5 and flip the Y coordinate
    shadowTexCoord.x = (1.0 + shadowTexCoord.x) * 0.25;
    shadowTexCoord.y = (1.0 - shadowTexCoord.y) * 0.25;

    // Bias the depth value (note, we need better depth bias settings)
    shadowTexCoord.z -= LightData[lightIndex].ShadowParams.x;

    float LinearZ = LinearizeDepth(shadowTexCoord.z, LightData[lightIndex].ShadowParams.y, LightData[lightIndex].ShadowParams.z);

    if ((shadowTexCoord.y < 0) || (shadowTexCoord.y > .5)) return 1.f;
    if ((shadowTexCoord.x < 0) || (shadowTexCoord.x > .5)) return 1.f;

    // offsets of the center of the shadow map atlas
    float offsetsX[4] = { 0.0, 1.0, 0.0, 1.0 };
    float offsetsY[4] = { 0.0, 0.0, 1.0, 1.0 };
    shadowTexCoord.x += offsetsX[LightData[lightIndex].ShadowMapIndex] * .5;
    shadowTexCoord.y += offsetsY[LightData[lightIndex].ShadowMapIndex] * .5;

    // Sample shadow map
    float shadow = 0.0;

    static const int kernelLevel = 2;
    static const int kernelWidth = 2 * kernelLevel + 1;
    [unroll] for (int i = -kernelLevel; i <= kernelLevel; i++)
    {
        [unroll] for (int j = -kernelLevel; j <= kernelLevel; j++)
        {
            float distToLight = ShadowTexture.Sample(LinearWrapSampler, shadowTexCoord.xy, int2(i, j)).r;
            float LinearSample = LinearizeDepth(distToLight, LightData[lightIndex].ShadowParams.y, LightData[lightIndex].ShadowParams.z);
            bool isLit = UseDepthApproximation ? LinearZ < LinearSample : shadowTexCoord.z < distToLight;

            if (isLit)
                shadow += 1.f;
            else
            {
                shadow += UseDepthApproximation ? ComputeShadowAttenuation(LinearZ - LinearSample, params.FiberSpacing, params.FiberRadius, params.HairShadowAlpha) : 0.f;
            }
        }
    }

    // Average the values according to number of samples
    shadow /= (kernelWidth * kernelWidth);
    
    return shadow;
}

float3 AccumulateHairLight(float3 vTangent, float3 vPositionWS, float3 vViewDirWS, in HairShadeParams params, float3 vNDC)
{
    // Initialize information needed for all lights
    float3 V = normalize(vViewDirWS);
    float3 T = normalize(vTangent);

    float3 color = float3(0.0, 0.0, 0.0);

    // Needed for each light calculation
    float3 LightVector;
    float LightIntensity;

    // Start with non-shadowed lights
    uint lightCount = min(NumLights, AMD_TRESSFX_MAX_LIGHTS);
    for (uint i = 0; i < lightCount; i++)
    {
        GetLightParams(vPositionWS, i, LightVector, LightIntensity, params);

        if (LightIntensity)
        {
            // Compute shadow term (if we've got one)
            float fShadowTerm = ComputeLightShadow(i, vPositionWS, params);

            if (fShadowTerm > 0.f)
            {
                float3 L = LightVector;
                float3 LightColor = LightData[i].LightColor;

                float3 reflection = ComputeDiffuseSpecFactors(V, L, T, params);

                float3 ReflectedLight = reflection.x * LightColor * params.Color;
                ReflectedLight += reflection.y * LightColor;
                ReflectedLight += reflection.z * LightColor * params.Color;
                ReflectedLight *= fShadowTerm * LightIntensity;

                color += max(float3(0, 0, 0), ReflectedLight);
            }
        }
    }

    return color;
}

