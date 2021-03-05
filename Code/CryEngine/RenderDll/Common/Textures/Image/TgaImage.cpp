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

#include "RenderDll_precompiled.h"
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzFramework/Archive/IArchive.h>

static AZ::IO::HandleType sFileHandle;

namespace ImageUtils
{
    string OutputPath(const char* filename)
    {
        char resolvedPath[AZ_MAX_PATH_LEN];
        if (!AZ::IO::FileIOBase::GetInstance()->ResolvePath(filename, resolvedPath, AZ_MAX_PATH_LEN))
        {
            return "";
        }
        string fullPath(resolvedPath);
        return fullPath;
    }
}

static void bwrite(unsigned char data)
{
    byte d[2];
    d[0] = data;

    gEnv->pCryPak->FWrite(d, 1, 1, sFileHandle);
}

void wwrite(unsigned short data)
{
    unsigned char h, l;

    l = data & 0xFF;
    h = data >> 8;
    bwrite(l);
    bwrite(h);
}


static void WritePixel(int depth, unsigned long a, unsigned long r, unsigned long g, unsigned long b)
{
    DWORD color16;

    switch (depth)
    {
    case 32:
        bwrite((byte)b);    // b
        bwrite((byte)g);    // g
        bwrite((byte)r);    // r
        bwrite((byte)a);    // a
        break;

    case 24:
        bwrite((byte)b);    // b
        bwrite((byte)g);    // g
        bwrite((byte)r);    // r
        break;

    case 16:
        r >>= 3;
        g >>= 3;
        b >>= 3;

        r &= 0x1F;
        g &= 0x1F;
        b &= 0x1F;

        color16 = (r << 10) | (g << 5) | b;

        wwrite((unsigned short)color16);
        break;
    }
}

static void GetPixel(const byte* data, int depth,  unsigned long& a, unsigned long& r, unsigned long& g, unsigned long& b)
{
    switch (depth)
    {
    case 32:
        r = *data++;
        g = *data++;
        b = *data++;
        a = *data++;
        break;

    case 24:
        r = *data++;
        g = *data++;
        b = *data++;
        a = 0xFF;
        break;

    default:
        assert(0);
        break;
    }
}

bool WriteTGA(const byte* data, int width, int height, const char* filename, int src_bits_per_pixel, int dest_bits_per_pixel)
{
#if defined(AZ_PLATFORMS_APPLE_IOS) || defined(AZ_PLATFORM_LINUX)
    return false;
#else
    int i;
    unsigned long r, g, b, a;

    if ((sFileHandle = gEnv->pCryPak->FOpen(filename, "wb")) == AZ::IO::InvalidHandle)
    {
        return false;
    }

    int id_length = 0;
    int x_org = 0;
    int y_org = 0;
    int desc = 0x20; // origin is upper left

    // 32 bpp

    int cm_index = 0;
    int cm_length = 0;
    int cm_entry_size = 0;
    int color_map_type = 0;

    int type = 2;

    bwrite(id_length);
    bwrite(color_map_type);
    bwrite(type);

    wwrite(cm_index);
    wwrite(cm_length);

    bwrite(cm_entry_size);

    wwrite(x_org);
    wwrite(y_org);
    wwrite((unsigned short)width);
    wwrite((unsigned short)height);

    bwrite(dest_bits_per_pixel);

    bwrite(desc);

    int hxw = height * width;

    int right = 0;

    const DWORD* temp_dp = (const DWORD*)data;      // data = input pointer

    const DWORD* swap = 0;

    UINT src_bytes_per_pixel = src_bits_per_pixel / 8;

    UINT size_in_bytes = hxw * src_bytes_per_pixel;

    if (src_bits_per_pixel == dest_bits_per_pixel)
    {
#if defined(NEED_ENDIAN_SWAP)
        byte* d = new byte [hxw * 4];
        memcpy(d, data, hxw * 4);
        SwapEndian((uint32*)d, hxw);
        gEnv->pCryPak->FWrite(d, hxw, src_bytes_per_pixel, sFileHandle);
        SAFE_DELETE_ARRAY(d);
#else
        gEnv->pCryPak->FWrite(data, hxw, src_bytes_per_pixel, sFileHandle);
#endif
    }
    else
    {
        for (i = 0; i < hxw; i++)
        {
            GetPixel(data, src_bits_per_pixel, a, b, g, r);
            WritePixel(dest_bits_per_pixel, a, b, g, r);
            data += src_bytes_per_pixel;
        }
    }

    gEnv->pCryPak->FClose(sFileHandle);

    SAFE_DELETE_ARRAY(swap);
    return true;
#endif
}

