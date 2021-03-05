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

#include <CryCommon/ImageExtensionHelper.h>

namespace CImageExtensionHelper
{
    ColorF GetAverageColor(uint8 const* pMem)
    {
        pMem = _findChunkStart(pMem, FOURCC_AvgC);

        if (pMem)
        {
            ColorF ret = ColorF(SwapEndianValue(*(uint32*)pMem));
            //flip red and blue
            const float cRed = ret.r;
            ret.r = ret.b;
            ret.b = cRed;
            return ret;
        }

        return Col_White;   // chunk does not exist
    }

    bool IsRangeless(ETEX_Format eTF)
    {
        return (eTF == eTF_BC6UH ||
            eTF == eTF_BC6SH ||
            eTF == eTF_R9G9B9E5 ||
            eTF == eTF_R16G16B16A16F ||
            eTF == eTF_R32G32B32A32F ||
            eTF == eTF_R16F ||
            eTF == eTF_R32F ||
            eTF == eTF_R16G16F ||
            eTF == eTF_R11G11B10F);
    }

    bool IsQuantized(ETEX_Format eTF)
    {
        return (eTF == eTF_B4G4R4A4      ||
                eTF == eTF_B5G6R5        ||
                eTF == eTF_B5G5R5        ||
                eTF == eTF_BC1           ||
                eTF == eTF_BC2           ||
                eTF == eTF_BC3           ||
                eTF == eTF_BC4U          ||
                eTF == eTF_BC4S          ||
                eTF == eTF_BC5U          ||
                eTF == eTF_BC5S          ||
                eTF == eTF_BC6UH         ||
                eTF == eTF_BC6SH         ||
                eTF == eTF_BC7           ||
                eTF == eTF_R9G9B9E5      ||
                eTF == eTF_ETC2          ||
                eTF == eTF_EAC_R11       ||
                eTF == eTF_ETC2A         ||
                eTF == eTF_EAC_RG11      ||
                eTF == eTF_PVRTC2        ||
                eTF == eTF_PVRTC4        ||
                eTF == eTF_ASTC_4x4 ||
                eTF == eTF_ASTC_5x4 ||
                eTF == eTF_ASTC_5x5 ||
                eTF == eTF_ASTC_6x5 ||
                eTF == eTF_ASTC_6x6 ||
                eTF == eTF_ASTC_8x5 ||
                eTF == eTF_ASTC_8x6 ||
                eTF == eTF_ASTC_8x8 ||
                eTF == eTF_ASTC_10x5 ||
                eTF == eTF_ASTC_10x6 ||
                eTF == eTF_ASTC_10x8 ||
                eTF == eTF_ASTC_10x10 ||
                eTF == eTF_ASTC_12x10 ||
                eTF == eTF_ASTC_12x12
                );
    }
}
