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

#ifdef ID_shadowMap
Texture2D                  shadowMap : register(TEX(ID_shadowMap));
SamplerComparisonState     samShadow : register(SMP(ID_shadowMap));
#endif

#ifdef ID_shadowBuffer
Texture2D                  shadowBuffer : register(TEX(ID_shadowBuffer));
SamplerState               sampleShadow : register(SMP(ID_shadowBuffer));
#endif

#ifdef ID_shadowMap
float FilterShadow(Texture2D shadowMap, float3 uv)
{
    float shadow = 0.0;

    static const int kernelLevel = 2;
    static const int kernelWidth = 2 * kernelLevel + 1;
    [unroll] for (int i = -kernelLevel; i <= kernelLevel; i++)
    {
        [unroll] for (int j = -kernelLevel; j <= kernelLevel; j++)
        {
            shadow += shadowMap.SampleCmpLevelZero(samShadow, uv.xy, uv.z, int2(i, j)).r;
        }
    }

    shadow /= (kernelWidth*kernelWidth); 
    return shadow;
}
#endif

float DoSpotShadow(in float3 vPosition, Light light)
{
#ifdef ID_shadowMap
    if (light.shadowMapIndex < 0)
        return 1.0;

    if (light.type != LightType_Spot)
        return 1.0; // no other light types cast shadows for now

    float4 shadowTexCoord = mul(light.mLightViewProj, float4(vPosition, 1));
    shadowTexCoord.xyz = shadowTexCoord.xyz / shadowTexCoord.w;

    // remember we are splitting the shadow map in 4 quarters 
    shadowTexCoord.x = (1.0 + shadowTexCoord.x) * 0.25;
    shadowTexCoord.y = (1.0 - shadowTexCoord.y) * 0.25;

    if ((shadowTexCoord.y < 0) || (shadowTexCoord.y > .5)) return 0;
    if ((shadowTexCoord.x < 0) || (shadowTexCoord.x > .5)) return 0;

    // offsets of the center of the shadow map atlas
    float offsetsX[4] = { 0.0, 1.0, 0.0, 1.0 };
    float offsetsY[4] = { 0.0, 0.0, 1.0, 1.0 };
    shadowTexCoord.x += offsetsX[light.shadowMapIndex] * .5;
    shadowTexCoord.y += offsetsY[light.shadowMapIndex] * .5;

    shadowTexCoord.z -= light.depthBias;
    
    return FilterShadow(shadowMap, shadowTexCoord.xyz);
#else
    return 1.0f;
#endif
}

float ReadShadows(in int2 screenPos, Light light)
{
#ifdef ID_shadowBuffer
    if (light.shadowMapIndex < 0)
        return 1.0f;
    int3 dims;
    shadowBuffer.GetDimensions(0, dims.x, dims.y, dims.z);
    const float2 uv = (screenPos + 0.5f) / float2(dims.xy);
    float4 shadows = shadowBuffer.SampleLevel(sampleShadow, uv, 0.0f);
    switch (light.shadowMapIndex)
    {
    case 0:
        return shadows.x;
    case 1:
        return shadows.y;
    case 2:
        return shadows.z;
    case 3:
        return shadows.w;
    default:
        break;
    }
    return 0;
#else
    return 1.0f;
#endif
}

float CalcShadows(in float3 worldPos, in int2 screenPos, Light light)
{
#ifdef ID_shadowBuffer
    return ReadShadows(screenPos, light);
#endif
#ifdef ID_shadowMap
    return DoSpotShadow(worldPos, light);
#endif
}
