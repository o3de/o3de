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

#include "stdafx.h"
#include "DDSLoader.h"
#include "DxgiFormatHelper.h"

struct DDS_PIXELFORMAT
{
    UINT32 size;
    UINT32 flags;
    UINT32 fourCC;
    UINT32 bitCount;
    UINT32 bitMaskR;
    UINT32 bitMaskG;
    UINT32 bitMaskB;
    UINT32 bitMaskA;
};

struct DDS_HEADER
{

    UINT32       dwSize;
    UINT32       dwHeaderFlags;
    UINT32       dwHeight;
    UINT32       dwWidth;
    UINT32       dwPitchOrLinearSize;
    UINT32       dwDepth;
    UINT32       dwMipMapCount;
    UINT32       dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    UINT32       dwSurfaceFlags;
    UINT32       dwCubemapFlags;
    UINT32       dwCaps3;
    UINT32       dwCaps4;
    UINT32       dwReserved2;
};

//--------------------------------------------------------------------------------------
// retrieve the GetDxGiFormat from a DDS_PIXELFORMAT
// based on http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
//--------------------------------------------------------------------------------------
static DXGI_FORMAT GetDxGiFormat(DDS_PIXELFORMAT pixelFmt)
{
    if (pixelFmt.flags & 0x00000004)   //DDPF_FOURCC
    {
        // Check for D3DFORMAT enums being set here
        switch (pixelFmt.fourCC)
        {
        case '1TXD':         return DXGI_FORMAT_BC1_UNORM;
        case '3TXD':         return DXGI_FORMAT_BC2_UNORM;
        case '5TXD':         return DXGI_FORMAT_BC3_UNORM;
        case 'U4CB':         return DXGI_FORMAT_BC4_UNORM;
        case 'A4CB':         return DXGI_FORMAT_BC4_SNORM;
        case '2ITA':         return DXGI_FORMAT_BC5_UNORM;
        case 'S5CB':         return DXGI_FORMAT_BC5_SNORM;
        case 'GBGR':         return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case 'BGRG':         return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case 36:             return DXGI_FORMAT_R16G16B16A16_UNORM;
        case 110:            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case 111:            return DXGI_FORMAT_R16_FLOAT;
        case 112:            return DXGI_FORMAT_R16G16_FLOAT;
        case 113:            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case 114:            return DXGI_FORMAT_R32_FLOAT;
        case 115:            return DXGI_FORMAT_R32G32_FLOAT;
        case 116:            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:             return DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        switch (pixelFmt.bitMaskR)
        {
        case 0xff:        return DXGI_FORMAT_R8G8B8A8_UNORM;
        case 0x00ff0000:  return DXGI_FORMAT_B8G8R8A8_UNORM;
        case 0xffff:      return DXGI_FORMAT_R16G16_UNORM;
        case 0x3ff:       return DXGI_FORMAT_R10G10B10A2_UNORM;
        case 0x7c00:      return DXGI_FORMAT_B5G5R5A1_UNORM;
        case 0xf800:      return DXGI_FORMAT_B5G6R5_UNORM;
        case 0:           return DXGI_FORMAT_A8_UNORM;
        default:          return DXGI_FORMAT_UNKNOWN;
        };
    }
}

HANDLE DDSLoad(const char *pFilename, IMG_INFO *pInfo)
{
    typedef enum RESOURCE_DIMENSION
    {
        RESOURCE_DIMENSION_UNKNOWN = 0,
        RESOURCE_DIMENSION_BUFFER = 1,
        RESOURCE_DIMENSION_TEXTURE1D = 2,
        RESOURCE_DIMENSION_TEXTURE2D = 3,
        RESOURCE_DIMENSION_TEXTURE3D = 4
    } RESOURCE_DIMENSION;

    typedef struct
    {
        DXGI_FORMAT      dxgiFormat;
        RESOURCE_DIMENSION  resourceDimension;
        UINT32           miscFlag;
        UINT32           arraySize;
        UINT32           reserved;
    } DDS_HEADER_DXT10;

    if (GetFileAttributesA(pFilename) == 0xFFFFFFFF)
        return INVALID_HANDLE_VALUE;

    HANDLE hFile = CreateFileA(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    LARGE_INTEGER largeFileSize;
    GetFileSizeEx(hFile, &largeFileSize);
    assert(0 == largeFileSize.HighPart);
    UINT32 fileSize = largeFileSize.LowPart;
    UINT32 rawTextureSize = fileSize;

    // read the header
    char headerData[4 + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)];
    DWORD dwBytesRead = 0;
    if (ReadFile(hFile, headerData, 4 + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10), &dwBytesRead, NULL))
    {
        char *pByteData = headerData;
        UINT32 dwMagic = *reinterpret_cast<UINT32 *>(pByteData);
        pByteData += 4;
        rawTextureSize -= 4;

        DDS_HEADER *header = reinterpret_cast<DDS_HEADER *>(pByteData);
        pByteData += sizeof(DDS_HEADER);
        rawTextureSize -= sizeof(DDS_HEADER);

        DDS_HEADER_DXT10 *header10 = NULL;
        if (dwMagic == '01XD')   // "DX10"
        {
            header10 = reinterpret_cast<DDS_HEADER_DXT10 *>(&pByteData[4]);
            pByteData += sizeof(DDS_HEADER_DXT10);
            rawTextureSize -= sizeof(DDS_HEADER_DXT10);
            header->ddspf.bitCount = (UINT32)BitsPerPixel(header10->dxgiFormat);

            IMG_INFO dx10header =
            {
                header->dwWidth,
                header->dwHeight,
                header->dwDepth,
                header10->arraySize,
                header->dwMipMapCount,
                header10->dxgiFormat,
                header->ddspf.bitCount
            };
            *pInfo = dx10header;
        }
        else if (dwMagic == ' SDD')   // "DDS "
        {
            // DXGI
            UINT32 arraySize = (header->dwCubemapFlags == 0xfe00) ? 6 : 1;
            DXGI_FORMAT dxgiFormat = GetDxGiFormat(header->ddspf);
            UINT32 mipMapCount = header->dwMipMapCount ? header->dwMipMapCount : 1;
            header->ddspf.bitCount = (UINT32)BitsPerPixel(dxgiFormat);

            IMG_INFO dx10header =
            {
                header->dwWidth,
                header->dwHeight,
                header->dwDepth ? header->dwDepth : 1,
                arraySize,
                mipMapCount,
                dxgiFormat,
                header->ddspf.bitCount
            };
            *pInfo = dx10header;
        }
        else
        {
            return INVALID_HANDLE_VALUE;
        }
    }

    SetFilePointer(hFile, fileSize - rawTextureSize, 0, FILE_BEGIN);

    return hFile;
}

bool DDSLoader::Load(const char *pFilename, float cutOff, IMG_INFO *pInfo)
{
    m_handle = DDSLoad(pFilename, pInfo);

    return m_handle != INVALID_HANDLE_VALUE;
}

void DDSLoader::CopyPixels(void *pDest, uint32_t stride, uint32_t bytesWidth, uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        ReadFile(m_handle, (char*)pDest + y*stride, bytesWidth, NULL, NULL);
    }
}
