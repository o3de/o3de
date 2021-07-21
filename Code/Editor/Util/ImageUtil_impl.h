/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_UTILIMAGEUTIL_IMPL_H
#define CRYINCLUDE_EDITOR_UTILIMAGEUTIL_IMPL_H
#pragma once

#include "Include/IImageUtil.h"

class CImageUtil_impl
    : public IImageUtil
{
public:
    CImageUtil_impl(){}
    ~CImageUtil_impl(){}

    //! Load image, detect image type by file extension.
    // Arguments:
    //   pQualityLoss - 0 if info is not needed, pointer to the result otherwise - not need to preinitialize
    virtual bool LoadImage(const QString& fileName, CImageEx& image, bool* pQualityLoss = 0) override;

    //! Save image, detect image type by file extension.
    virtual bool SaveImage(const QString& fileName, CImageEx& image) override;

    // General image fucntions
    virtual bool LoadJPEG(const QString& strFileName, CImageEx& image) override;

    virtual bool SaveJPEG(const QString& strFileName, CImageEx& image) override;

    virtual bool SaveBitmap(const QString& szFileName, CImageEx& image) override;

    virtual bool LoadBmp(const QString& file, CImageEx& image) override;

    virtual bool SavePGM(const QString& fileName, const CImageEx& image) override;

    virtual bool LoadPGM(const QString& fileName, CImageEx& image) override;

    //! Scale source image to fit size of target image.
    virtual void ScaleToFit(const CByteImage& srcImage, CByteImage& trgImage) override;

    //! Scale source image to fit size of target image.
    virtual void ScaleToFit(const CImageEx& srcImage, CImageEx& trgImage) override;

    //! Scale source image to fit twice side by side in target image.
    virtual void ScaleToDoubleFit(const CImageEx& srcImage, CImageEx& trgImage) override;

    //! Scale source image twice down image with filtering
    enum _EAddrMode
    {
        WRAP, CLAMP
    };
    virtual void DownScaleSquareTextureTwice(const CImageEx& srcImage, CImageEx& trgImage,
        IImageUtil::_EAddrMode eAddressingMode = IImageUtil::WRAP) override;

    //! Smooth image.
    virtual void SmoothImage(CByteImage& image, int numSteps) override;

    //! behavior outside of the texture is not defined
    //! \param iniX in fix point 24.8
    //! \param iniY in fix point 24.8
    //! \return 0..255
    virtual unsigned char GetBilinearFilteredAt(const int iniX256, const int iniY256, const CByteImage& image) override;
};
#endif // CRYINCLUDE_EDITOR_UTILIMAGEUTIL_IMPL_H
