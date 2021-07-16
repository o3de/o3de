/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ImageUtil_impl.h"

// Editor
#include "Util/ImageUtil.h"

bool CImageUtil_impl::LoadImage(const QString& fileName, CImageEx& image, bool* pQualityLoss)
{
    return CImageUtil::LoadImage(fileName, image, pQualityLoss);
}

bool CImageUtil_impl::SaveImage(const QString& fileName, CImageEx& image)
{
    return CImageUtil::SaveImage(fileName, image);
}

bool CImageUtil_impl::LoadJPEG(const QString& strFileName, CImageEx& image)
{
    return CImageUtil::LoadJPEG(strFileName, image);
}

bool CImageUtil_impl::SaveJPEG(const QString& strFileName, CImageEx& image)
{
    return CImageUtil::SaveJPEG(strFileName, image);
}

bool CImageUtil_impl::SaveBitmap(const QString& szFileName, CImageEx& image)
{
    return CImageUtil::SaveBitmap(szFileName, image);
}

bool CImageUtil_impl::LoadBmp(const QString& file, CImageEx& image)
{
    return CImageUtil::LoadBmp(file, image);
}

bool CImageUtil_impl::SavePGM(const QString& fileName, const CImageEx& image)
{
    return CImageUtil::SavePGM(fileName, image);
}

bool CImageUtil_impl::LoadPGM(const QString& fileName, CImageEx& image)
{
    return CImageUtil::LoadPGM(fileName, image);
}

void CImageUtil_impl::ScaleToFit(const CByteImage& srcImage, CByteImage& trgImage)
{
    CImageUtil::ScaleToFit(srcImage, trgImage);
}

void CImageUtil_impl::ScaleToFit(const CImageEx& srcImage, CImageEx& trgImage)
{
    CImageUtil::ScaleToFit(srcImage, trgImage);
}

void CImageUtil_impl::ScaleToDoubleFit(const CImageEx& srcImage, CImageEx& trgImage)
{
    CImageUtil::ScaleToDoubleFit(srcImage, trgImage);
}

void CImageUtil_impl::DownScaleSquareTextureTwice(const CImageEx& srcImage, CImageEx& trgImage, IImageUtil::_EAddrMode eAddressingMode)
{
    CImageUtil::DownScaleSquareTextureTwice(srcImage, trgImage, eAddressingMode);
}

void CImageUtil_impl::SmoothImage(CByteImage& image, int numSteps)
{
    CImageUtil::SmoothImage(image, numSteps);
}

unsigned char CImageUtil_impl::GetBilinearFilteredAt(const int iniX256, const int iniY256, const CByteImage& image)
{
    return CImageUtil::GetBilinearFilteredAt(iniX256, iniY256, image);
}
