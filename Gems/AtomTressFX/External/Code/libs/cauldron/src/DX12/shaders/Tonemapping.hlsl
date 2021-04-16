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

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
    float u_exposure         : packoffset(c0.x);
    int   u_toneMapper       : packoffset(c0.y);    
    int   u_applyGamma       : packoffset(c0.z);
}

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
struct VERTEX
{
    float2 vTexcoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D        HDR              :register(t0);
SamplerState     samLinearWrap    :register(s0);

//--------------------------------------------------------------------------------------
// Timothy Lottes tone mapper
//--------------------------------------------------------------------------------------
// General tonemapping operator, build 'b' term.
float ColToneB(float hdrMax, float contrast, float shoulder, float midIn, float midOut) 
{
    return
        -((-pow(midIn, contrast) + (midOut*(pow(hdrMax, contrast*shoulder)*pow(midIn, contrast) -
            pow(hdrMax, contrast)*pow(midIn, contrast*shoulder)*midOut)) /
            (pow(hdrMax, contrast*shoulder)*midOut - pow(midIn, contrast*shoulder)*midOut)) /
            (pow(midIn, contrast*shoulder)*midOut));
}

// General tonemapping operator, build 'c' term.
float ColToneC(float hdrMax, float contrast, float shoulder, float midIn, float midOut) 
{
    return (pow(hdrMax, contrast*shoulder)*pow(midIn, contrast) - pow(hdrMax, contrast)*pow(midIn, contrast*shoulder)*midOut) /
           (pow(hdrMax, contrast*shoulder)*midOut - pow(midIn, contrast*shoulder)*midOut);
}

// General tonemapping operator, p := {contrast,shoulder,b,c}.
float ColTone(float x, float4 p) 
{ 
    float z = pow(x, p.r); 
    return z / (pow(z, p.g)*p.b + p.a); 
}

float3 TimothyTonemapper(float3 color)
{
    static float hdrMax = 16.0; // How much HDR range before clipping. HDR modes likely need this pushed up to say 25.0.
    static float contrast = 2.0; // Use as a baseline to tune the amount of contrast the tonemapper has.
    static float shoulder = 1.0; // Likely don’t need to mess with this factor, unless matching existing tonemapper is not working well..
    static float midIn = 0.18; // most games will have a {0.0 to 1.0} range for LDR so midIn should be 0.18.
    static float midOut = 0.18; // Use for LDR. For HDR10 10:10:10:2 use maybe 0.18/25.0 to start. For scRGB, I forget what a good starting point is, need to re-calculate.

    float b = ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
    float c = ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

    #define EPS 1e-6f
    float peak = max(color.r, max(color.g, color.b));
    peak = max(EPS, peak); //avoid dividing by zero

    float3 ratio = color / peak;
    peak = ColTone(peak, float4(contrast, shoulder, b, c) );
    // then process ratio

    // probably want send these pre-computed (so send over saturation/crossSaturation as a constant)
    float crosstalk = 4.0; // controls amount of channel crosstalk
    float saturation = contrast; // full tonal range saturation control
    float crossSaturation = contrast*16.0; // crosstalk saturation

    float white = 1.0;

    // wrap crosstalk in transform
    ratio = pow(abs(ratio), saturation / crossSaturation);
    ratio = lerp(ratio, white, pow(peak, crosstalk));
    ratio = pow(abs(ratio), crossSaturation);

    // then apply ratio to peak
    color = peak * ratio;
    return color;
}

//--------------------------------------------------------------------------------------
// The tone mapper used in HDRToneMappingCS11
//--------------------------------------------------------------------------------------
float3 DX11DSK(float3 color)
{
    float  MIDDLE_GRAY = 0.72f;
    float  LUM_WHITE = 1.5f;

    // Tone mapping
    color.rgb *= MIDDLE_GRAY;
    color.rgb *= (1.0f + color/LUM_WHITE);
    color.rgb /= (1.0f + color);
    
    return color;
}

//--------------------------------------------------------------------------------------
// Reinhard
//--------------------------------------------------------------------------------------
float3 Reinhard(float3 color)
{
    return color/(1+color);
}

//--------------------------------------------------------------------------------------
// Hable's filmic
//--------------------------------------------------------------------------------------
float3 Uncharted2TonemapOp(float3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float3 Uncharted2Tonemap(float3 color)
{
    float W = 11.2;    
    return Uncharted2TonemapOp(2.0 * color) / Uncharted2TonemapOp(W);
}

//--------------------------------------------------------------------------------------
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
//--------------------------------------------------------------------------------------
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x + b)) / (x*(c*x + d) + e));
}

float3 Pattern(float2 vTexcoord)
{
    if (vTexcoord.x < .5)
        return float3(.5, .5, .5);

    uint y = vTexcoord.y * 720;
    if ((y&1)==1)
        return float3(1.0, 1.0, 1.0);


    return float3(0, 0, 0);
}

float3 ApplyGamma(float3 color)
{
    return pow(abs(color.rgb), 1.0f / 2.2f);    
}

float3 Tonemap(float3 color, float exposure, int tonemapper)
{
    color *= exposure;

    switch (tonemapper)
    {
    case 0: return TimothyTonemapper(color);
    case 1: return DX11DSK(color);
    case 2: return Reinhard(color);
    case 3: return Uncharted2Tonemap(color);
    case 4: return ACESFilm(color);
    case 5: return color;
    default: return float3(1, 1, 1);
    }
}

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

float4 mainPS(VERTEX Input) : SV_Target
{
    if (u_exposure<0)
    {
        return HDR.Sample(samLinearWrap, Input.vTexcoord);
    }

    float4 texColor = HDR.Sample(samLinearWrap, Input.vTexcoord);

    float3 color = Tonemap(texColor.rgb, u_exposure, u_toneMapper);

    // We don't need to apply the gamma because the swapchain format is SRGB
    //
    if (u_applyGamma > 0)
    {
        color = ApplyGamma(color);
    }

    return float4(color, 1);
}
