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

// Description : Image implementation,


#include "EditorDefs.h"

#include "Image.h"


bool CImageEx::ConvertToFloatImage(CFloatImage& dstImage)
{
    uint32 pixelMask;
    float intToFloat;

    switch (GetFormat())
    {
    case eTF_Unknown:
    case eTF_R8G8B8A8:
        pixelMask = std::numeric_limits<uint8>::max();
        intToFloat = static_cast<float>(pixelMask);
        break;
    case eTF_R16G16:
        pixelMask = std::numeric_limits<uint16>::max();
        intToFloat = static_cast<float>(pixelMask);
        break;
    default:
        return false;
        break;
    }

    dstImage.Allocate(GetWidth(), GetHeight());

    uint32* srcPixel = GetData();
    float* dstPixel = dstImage.GetData();

    for (int pixel = 0; pixel < (GetHeight() * GetWidth()); pixel++)
    {
        dstPixel[pixel] = clamp_tpl(static_cast<float>(srcPixel[pixel] & pixelMask) / intToFloat, 0.0f, 1.0f);
    }

    return true;
}

void CImageEx::SwapRedAndBlue()
{
    if (!IsValid())
    {
        return;
    }
    // Set the loop pointers
    uint32* pPixData = GetData();
    uint32* pPixDataEnd = pPixData + GetWidth() * GetHeight();
    // Switch R and B
    while (pPixData != pPixDataEnd)
    {
        // Extract the bits, shift them, put them back and advance to the next pixel
        *pPixData = (*pPixData & 0xFF000000) | ((*pPixData & 0x00FF0000) >> 16) | (*pPixData & 0x0000FF00) | ((*pPixData & 0x000000FF) << 16);
        ++pPixData;
    }
}

void CImageEx::ReverseUpDown()
{
    if (!IsValid())
    {
        return;
    }

    uint32* pPixData = GetData();
    uint32* pReversePix = new uint32[GetWidth() * GetHeight()];

    for (int i = GetHeight() - 1, i2 = 0; i >= 0; i--, i2++)
    {
        for (int k = 0; k < GetWidth(); k++)
        {
            pReversePix[i2 * GetWidth() + k] = pPixData[i * GetWidth() + k];
        }
    }

    Attach(pReversePix, GetWidth(), GetHeight());
}

void CImageEx::FillAlpha(unsigned char value)
{
    if (!IsValid())
    {
        return;
    }
    // Set the loop pointers
    uint32* pPixData = GetData();
    uint32* pPixDataEnd = pPixData + GetWidth() * GetHeight();
    while (pPixData != pPixDataEnd)
    {
        *pPixData = (*pPixData & 0x00FFFFFF) | (value << 24);
        ++pPixData;
    }
}
