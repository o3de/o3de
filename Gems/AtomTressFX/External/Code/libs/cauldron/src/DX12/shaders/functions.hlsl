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

static const float M_PI = 3.141592653589793;
static const float c_MinReflectance = 0.04;

struct AngularInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector

    float VdotH;                  // cos angle between view direction and half vector

    float3 padding;
};

float getPerceivedBrightness(float3 vec)
{
    return sqrt(0.299 * vec.r * vec.r + 0.587 * vec.g * vec.g + 0.114 * vec.b * vec.b);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34
float solveMetallic(float3 diffuse, float3 specular, float oneMinusSpecularStrength) {
    float specularBrightness = getPerceivedBrightness(specular);

    if (specularBrightness < c_MinReflectance) {
        return 0.0;
    }

    float diffuseBrightness = getPerceivedBrightness(diffuse);

    float a = c_MinReflectance;
    float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;
    float c = c_MinReflectance - specularBrightness;
    float D = b * b - 4.0 * a * c;

    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

AngularInfo getAngularInfo(float3 pointToLight, float3 normal, float3 view)
{
    // Standard one-letter names
    float3 n = normalize(normal);           // Outward direction of surface point
    float3 v = normalize(view);             // Direction from surface point to view
    float3 l = normalize(pointToLight);     // Direction from surface point to light
    float3 h = normalize(l + v);            // Direction of the vector between l and v

    float NdotL = clamp(dot(n, l), 0.0, 1.0);
    float NdotV = clamp(dot(n, v), 0.0, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    AngularInfo angularInfo = {
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        float3(0, 0, 0)
    };

    return angularInfo;
}