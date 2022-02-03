/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    const int height = GetHeight();
    const int width = GetWidth(); 
    for (int i = 0; i < height / 2; i++)
    {
        for (int j = 0; j < width; j++)
        {
            AZStd::swap(pPixData[i * width + j], pPixData[(height - 1 - i) * width + j]);
        }
    }

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
