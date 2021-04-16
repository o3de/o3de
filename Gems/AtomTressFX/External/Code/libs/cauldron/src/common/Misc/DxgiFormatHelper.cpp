// AMD AMDUtils code
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
// File: LoaderHelpers.h
//
// Helper functions for texture loaders and screen grabber
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "DxgiFormatHelper.h"
#include <assert.h>

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
size_t BitsPerPixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    default:
        return 0;
    }
}

//--------------------------------------------------------------------------------------
// return the byte size of a pixel (or block if block compressed)
//--------------------------------------------------------------------------------------
size_t GetPixelByteSize(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case(DXGI_FORMAT_R10G10B10A2_TYPELESS):
    case(DXGI_FORMAT_R10G10B10A2_UNORM):
    case(DXGI_FORMAT_R10G10B10A2_UINT):
    case(DXGI_FORMAT_R11G11B10_FLOAT):
    case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
    case(DXGI_FORMAT_R8G8B8A8_UNORM):
    case(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
    case(DXGI_FORMAT_R8G8B8A8_UINT):
    case(DXGI_FORMAT_R8G8B8A8_SNORM):
    case(DXGI_FORMAT_R8G8B8A8_SINT):
    case(DXGI_FORMAT_B8G8R8A8_UNORM):
    case(DXGI_FORMAT_B8G8R8X8_UNORM):
    case(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM):
    case(DXGI_FORMAT_B8G8R8A8_TYPELESS):
    case(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB):
    case(DXGI_FORMAT_B8G8R8X8_TYPELESS):
    case(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB):
    case(DXGI_FORMAT_R16G16_TYPELESS):
    case(DXGI_FORMAT_R16G16_FLOAT):
    case(DXGI_FORMAT_R16G16_UNORM):
    case(DXGI_FORMAT_R16G16_UINT):
    case(DXGI_FORMAT_R16G16_SNORM):
    case(DXGI_FORMAT_R16G16_SINT):
    case(DXGI_FORMAT_R32_TYPELESS):
    case(DXGI_FORMAT_D32_FLOAT):
    case(DXGI_FORMAT_R32_FLOAT):
    case(DXGI_FORMAT_R32_UINT):
    case(DXGI_FORMAT_R32_SINT):
    return 4;

    case(DXGI_FORMAT_BC1_TYPELESS):
    case(DXGI_FORMAT_BC1_UNORM):
    case(DXGI_FORMAT_BC1_UNORM_SRGB):
    case(DXGI_FORMAT_BC4_TYPELESS):
    case(DXGI_FORMAT_BC4_UNORM):
    case(DXGI_FORMAT_BC4_SNORM):
    case(DXGI_FORMAT_R16G16B16A16_FLOAT):
    case(DXGI_FORMAT_R16G16B16A16_TYPELESS):
        return 8;

    case(DXGI_FORMAT_BC2_TYPELESS):
    case(DXGI_FORMAT_BC2_UNORM):
    case(DXGI_FORMAT_BC2_UNORM_SRGB):
    case(DXGI_FORMAT_BC3_TYPELESS):
    case(DXGI_FORMAT_BC3_UNORM):
    case(DXGI_FORMAT_BC3_UNORM_SRGB):
    case(DXGI_FORMAT_BC5_TYPELESS):
    case(DXGI_FORMAT_BC5_UNORM):
    case(DXGI_FORMAT_BC5_SNORM):
    case(DXGI_FORMAT_BC6H_TYPELESS):
    case(DXGI_FORMAT_BC6H_UF16):
    case(DXGI_FORMAT_BC6H_SF16):
    case(DXGI_FORMAT_BC7_TYPELESS):
    case(DXGI_FORMAT_BC7_UNORM):
    case(DXGI_FORMAT_BC7_UNORM_SRGB):
    case(DXGI_FORMAT_R32G32B32A32_FLOAT):
    case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
        return 16;

    default:
        assert(0);
        break;
    }
    return 0;
}



//--------------------------------------------------------------------------------------
// Convert a *_SRGB format into a non-gamma one
//--------------------------------------------------------------------------------------
DXGI_FORMAT ConvertIntoNonGammaFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM;
    }

    return format;
}

DXGI_FORMAT ConvertIntoGammaFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
    case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
    }

    return format;
}

DXGI_FORMAT SetFormatGamma(DXGI_FORMAT format, bool addGamma)
{
    if (addGamma)
    {
        format = ConvertIntoGammaFormat(format);
    }
    else
    {
        format = ConvertIntoNonGammaFormat(format);
    }

    return format;
}
