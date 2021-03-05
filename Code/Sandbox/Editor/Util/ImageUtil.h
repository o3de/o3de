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

// Description : Image utilities.


#pragma once

#include <Include/IImageUtil.h>

/*!
 *  Utility Class to manipulate images.
 */
class SANDBOX_API CImageUtil
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Image loading.
    //////////////////////////////////////////////////////////////////////////
    //! Load image, detect image type by file extension.
    // Arguments:
    //   pQualityLoss - 0 if info is not needed, pointer to the result otherwise - not need to preinitialize
    static bool LoadImage(const QString& fileName, CImageEx& image, bool* pQualityLoss = 0);
    //! Save image, detect image type by file extension.
    static bool SaveImage(const QString& fileName, CImageEx& image);

    // General image fucntions
    static bool LoadJPEG(const QString& strFileName, CImageEx& image);
    static bool SaveJPEG(const QString& strFileName, CImageEx& image);

    static bool SaveBitmap(const QString& szFileName, CImageEx& image);
    static bool LoadBmp(const QString& file, CImageEx& image);

    //! Save image in PGM format.
    static bool SavePGM(const QString& fileName, const CImageEx& image);
    //! Load image in PGM format.
    static bool LoadPGM(const QString& fileName, CImageEx& image);

    //////////////////////////////////////////////////////////////////////////
    // Image scaling.
    //////////////////////////////////////////////////////////////////////////
    //! Scale source image to fit size of target image.
    static void ScaleToFit(const CByteImage& srcImage, CByteImage& trgImage);
    //! Scale source image to fit size of target image.
    static void ScaleToFit(const CImageEx& srcImage, CImageEx& trgImage);
    //! Scale source image to fit twice side by side in target image.
    static void ScaleToDoubleFit(const CImageEx& srcImage, CImageEx& trgImage);
    //! Scale source image twice down image with filering

    static void DownScaleSquareTextureTwice(const CImageEx& srcImage, CImageEx& trgImage, IImageUtil::_EAddrMode eAddressingMode = IImageUtil::WRAP);

    //! Smooth image.
    static void SmoothImage(CByteImage& image, int numSteps);

    //////////////////////////////////////////////////////////////////////////
    // filtered lookup
    //////////////////////////////////////////////////////////////////////////

    //! behaviour outside of the texture is not defined
    //! \param iniX in fix point 24.8
    //! \param iniY in fix point 24.8
    //! \return 0..255
    static unsigned char GetBilinearFilteredAt(const int iniX256, const int iniY256, const CByteImage& image);

    static bool QImageToImage(const QImage& bitmap, CImageEx& image);
    static bool ImageToQImage(const CImageEx& image, QImage& bitmapObj);

private:
    static bool Load(const QString& fileName, CImageEx& image);
    static bool Save(const QString& strFileName, CImageEx& inImage);
};

