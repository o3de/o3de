/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_INCLUDE_IIMAGEUTIL_H
#define CRYINCLUDE_EDITOR_INCLUDE_IIMAGEUTIL_H
#pragma once

#include "Util/Image.h"
class CAlphaBitmap;

struct IImageUtil
{
    virtual ~IImageUtil() = default;
    
    //! Load image, detect image type by file extension.
    // Arguments:
    //   pQualityLoss - 0 if info is not needed, pointer to the result otherwise - not need to preinitialize
    virtual bool LoadImage(const QString& fileName, CImageEx& image, bool* pQualityLoss = 0) = 0;

    //! Save image, detect image type by file extension.
    virtual bool SaveImage(const QString& fileName, CImageEx& image) = 0;

    // General image fucntions
    virtual bool LoadJPEG(const QString& strFileName, CImageEx& image) = 0;

    virtual bool SaveJPEG(const QString& strFileName, CImageEx& image) = 0;

    virtual bool SaveBitmap(const QString& szFileName, CImageEx& image) = 0;

    virtual bool LoadBmp(const QString& file, CImageEx& image) = 0;

    virtual bool SavePGM(const QString& fileName, const CImageEx& image) = 0;

    virtual bool LoadPGM(const QString& fileName, CImageEx& image) = 0;

    //! Scale source image to fit size of target image.
    virtual void ScaleToFit(const CByteImage& srcImage, CByteImage& trgImage) = 0;

    //! Scale source image to fit size of target image.
    virtual void ScaleToFit(const CImageEx& srcImage, CImageEx& trgImage) = 0;

    //! Scale source image to fit twice side by side in target image.
    virtual void ScaleToDoubleFit(const CImageEx& srcImage, CImageEx& trgImage) = 0;

    //! Scale source image twice down image with filering
    enum _EAddrMode
    {
        WRAP, CLAMP
    };
    virtual void DownScaleSquareTextureTwice(const CImageEx& srcImage, CImageEx& trgImage, _EAddrMode eAddressingMode = WRAP) = 0;

    //! Smooth image.
    virtual void SmoothImage(CByteImage& image, int numSteps) = 0;

    //! behavior outside of the texture is not defined
    //! \param iniX in fix point 24.8
    //! \param iniY in fix point 24.8
    //! \return 0..255
    virtual unsigned char GetBilinearFilteredAt(const int iniX256, const int iniY256, const CByteImage& image) = 0;
};
#endif // CRYINCLUDE_EDITOR_INCLUDE_IIMAGEUTIL_H
