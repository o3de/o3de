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
#pragma once

namespace ImageProcessing
{
    enum EPixelFormat : int
    {
        //unsigned formats
        ePixelFormat_R8G8B8A8 = 0,
        ePixelFormat_R8G8B8X8,
        ePixelFormat_R8G8,
        ePixelFormat_R8,
        ePixelFormat_A8,
        ePixelFormat_R16G16B16A16,
        ePixelFormat_R16G16,
        ePixelFormat_R16,

        //Custom FourCC Formats
        // Data in these FourCC formats is custom compressed data and only decodable by certain hardware.
        //ASTC formats supported by ios devices with A8 processor. Also supported by Android Extension Pack.
        ePixelFormat_ASTC_4x4,
        ePixelFormat_ASTC_5x4,
        ePixelFormat_ASTC_5x5,
        ePixelFormat_ASTC_6x5,
        ePixelFormat_ASTC_6x6,
        ePixelFormat_ASTC_8x5,
        ePixelFormat_ASTC_8x6,
        ePixelFormat_ASTC_8x8,
        ePixelFormat_ASTC_10x5,
        ePixelFormat_ASTC_10x6,
        ePixelFormat_ASTC_10x8,
        ePixelFormat_ASTC_10x10,
        ePixelFormat_ASTC_12x10,
        ePixelFormat_ASTC_12x12,
        //Formats supported by PowerVR GPU. Mainly for ios devices.
        ePixelFormat_PVRTC2,        //2bpp
        ePixelFormat_PVRTC4,        //4bpp
        //formats for opengl and opengles 3.0 (android devices)
        ePixelFormat_EAC_R11,       //one channel unsigned data
        ePixelFormat_EAC_RG11,      //two channel unsigned data
        ePixelFormat_ETC2,          //Compresses RGB888 data, it taks 4x4 groups of pixel data and compresses each into a 64-bit
        ePixelFormat_ETC2a,         //Compresses RGBA8888 data with full alpha support

        // Standardized Compressed DXGI Formats (DX10+)
        // Data in these compressed formats is hardware decodable on all DX10 chips, and manageable with the DX10-API.
        ePixelFormat_BC1,       // RGB without alpha, 0.5 byte/px
        ePixelFormat_BC1a,      // RGB with 1 bit of alpha, 0.5 byte/px.
        ePixelFormat_BC3,       // RGBA 1 byte/px, color maps with full alpha. 
        ePixelFormat_BC3t,      // BC3 with alpha weighted color 
        ePixelFormat_BC4,       // One color channel, 0.5 byte/px, unsigned
        ePixelFormat_BC4s,      // BC4, signed
        ePixelFormat_BC5,       // Two color channels, 1 byte/px, unsigned. Usually use for tangent-space normal maps
        ePixelFormat_BC5s,      // BC5, signed
        ePixelFormat_BC6UH,     // RGB, floating-point. Used for HDR images. Decompress to RGB in half floating point
        ePixelFormat_BC7,       // RGB or RGBA. 1 byte/px. Three color channels (4 to 7 bits per channel) with 0 to 8 bits of alpha
        ePixelFormat_BC7t,      // BC& with alpha weighted color 

        // Float formats
        // Data in a Float format is floating point data.
        ePixelFormat_R9G9B9E5,
        ePixelFormat_R32G32B32A32F,
        ePixelFormat_R32G32F,
        ePixelFormat_R32F,
        ePixelFormat_R16G16B16A16F,
        ePixelFormat_R16G16F,
        ePixelFormat_R16F,

        //legacy format. Only used to load old converted dds files. 
        ePixelFormat_B8G8R8A8,   //32bits rgba format

        ePixelFormat_R32,

        ePixelFormat_Count,
        ePixelFormat_Unknown = ePixelFormat_Count
    };

    inline bool IsASTCFormat(EPixelFormat fmt)
    {
        return fmt == ePixelFormat_ASTC_4x4   || fmt == ePixelFormat_ASTC_5x4  || fmt == ePixelFormat_ASTC_5x5   ||
               fmt == ePixelFormat_ASTC_6x5   || fmt == ePixelFormat_ASTC_6x6  || fmt == ePixelFormat_ASTC_8x5   ||
               fmt == ePixelFormat_ASTC_8x6   || fmt == ePixelFormat_ASTC_8x8  || fmt == ePixelFormat_ASTC_10x5  ||
               fmt == ePixelFormat_ASTC_10x6  || fmt == ePixelFormat_ASTC_10x8 || fmt == ePixelFormat_ASTC_10x10 ||
               fmt == ePixelFormat_ASTC_12x10 || fmt == ePixelFormat_ASTC_12x12;
    }

    inline bool IsETCFormat(EPixelFormat fmt)
    {
        return fmt == ePixelFormat_ETC2 || fmt == ePixelFormat_ETC2a || fmt == ePixelFormat_EAC_R11 ||
               fmt == ePixelFormat_EAC_RG11;
    }

    inline bool IsPVRTCFormat(EPixelFormat fmt)
    {
        return fmt == ePixelFormat_PVRTC2 || fmt == ePixelFormat_PVRTC4;
    }

} // namespace ImageProcessing
