//---------------------------------------------------------------------------------------
// Shader code utilities for TressFX
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
// Cutoff to not render hair
#define SHORTCUT_MIN_ALPHA 0.02

#define TRESSFX_FLOAT_EPSILON 1e-7

//--------------------------------------------------------------------------------------
//
//     Controls whether you do mul(M,v) or mul(v,M)
//     i.e., row major vs column major
//
//--------------------------------------------------------------------------------------
float4 MatrixMult(float4x4 m, float4 v)
{
    return mul(m, v);
}

// Calculate screen UV from XY NDC pair
float2 NDCToScreenUV(float2 vNDC)
{
    float2 ScreenUV = (vNDC + 1) / 2;
    ScreenUV.y = 1 - ScreenUV.y;
    return ScreenUV;
}

// Get world position from NDC coordinates
float3 NDCToWorld(float3 vNDC, float4x4 mInvViewProj)
{
    float4 pos = MatrixMult(mInvViewProj, float4(vNDC, 1));
    return pos.xyz / pos.w;
}

// Get NDC from screen position (and depth)
float3 ScreenPosToNDC(float3 vScreenPos, float4 viewport)
{
    float2 xy = vScreenPos.xy;

    // add viewport offset.
    xy += viewport.xy;

    // scale by viewport to put in 0 to 1
    xy /= viewport.zw;

    // shift and scale to put in -1 to 1. y is also being flipped.
    xy.x = (2 * xy.x) - 1;
    xy.y = 1 - (2 * xy.y);

    return float3(xy, vScreenPos.z);
}

// Pack a float4 into an uint
uint PackFloat4IntoUint(float4 vValue)
{
    return (((uint)(vValue.x * 255)) << 24) | (((uint)(vValue.y * 255)) << 16) | (((uint)(vValue.z * 255)) << 8) | (uint)(vValue.w * 255);
}

// Unpack a uint into a float4 value
float4 UnpackUintIntoFloat4(uint uValue)
{
    return float4(((uValue & 0xFF000000) >> 24) / 255.0, ((uValue & 0x00FF0000) >> 16) / 255.0, ((uValue & 0x0000FF00) >> 8) / 255.0, ((uValue & 0x000000FF)) / 255.0);
}

// Pack a float3 and a uint8 into an uint
uint PackFloat3ByteIntoUint(float3 vValue, uint uByteValue)
{
    return (((uint)(vValue.x * 255)) << 24) | (((uint)(vValue.y * 255)) << 16) | (((uint)(vValue.z * 255)) << 8) | uByteValue;
}

// Unpack a uint into a float3 and a uint8 value
float3 UnpackUintIntoFloat3Byte(uint uValue, out uint uByteValue)
{
    uByteValue = uValue & 0x000000FF;
    return float3(((uValue & 0xFF000000) >> 24) / 255.0, ((uValue & 0x00FF0000) >> 16) / 255.0, ((uValue & 0x0000FF00) >> 8) / 255.0);
}

//--------------------------------------------------------------------------------------
//
//      Safe_normalize-float2
//
//--------------------------------------------------------------------------------------
float2 Safe_normalize(float2 vec)
{
    float len = length(vec);
    return len >= TRESSFX_FLOAT_EPSILON ? (vec * rcp(len)) : float2(0, 0);
}

//--------------------------------------------------------------------------------------
//
//      Safe_normalize-float3
//
//--------------------------------------------------------------------------------------
float3 Safe_normalize(float3 vec)
{
    float len = length(vec);
    return len >= TRESSFX_FLOAT_EPSILON ? (vec * rcp(len)) : float3(0, 0, 0);
}