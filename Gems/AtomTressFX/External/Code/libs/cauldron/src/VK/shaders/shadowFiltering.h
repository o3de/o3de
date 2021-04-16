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
layout(set = 1, binding = ID_shadowMap) uniform sampler2D u_shadowMap;
#endif

// shadowmap filtering
float FilterShadow(vec3 uv)
{
    float shadow = 0.0;
#ifdef ID_shadowMap
    ivec2 texDim = textureSize(u_shadowMap, 0);
    float scale = 1.0;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);


    int kernelLevel = 2;
    int kernelWidth = 2 * kernelLevel + 1;
    for (int i = -kernelLevel; i <= kernelLevel; i++)
    {
        for (int j = -kernelLevel; j <= kernelLevel; j++)
        {
            float distanceFromLight = texture(u_shadowMap, uv.st + vec2(dx*i, dy*j)).r;                
            shadow += (distanceFromLight < uv.z) ? 0.0 : 1.0 ;
        }
    }

    shadow /= (kernelWidth*kernelWidth);
#endif
    return shadow;
}

//
// Project world space point onto shadowmap
//
float DoSpotShadow(vec3 vPosition, Light light)
{
#ifdef ID_shadowMap
    if (light.shadowMapIndex < 0)
        return 1.0f;

    vec4 shadowTexCoord = light.mLightViewProj * vec4(vPosition, 1.0);
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
    
    return FilterShadow(shadowTexCoord.xyz);
#else
    return 1.0f;
#endif
}
