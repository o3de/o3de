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

#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGE_DXTC_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGE_DXTC_H
#pragma once

#include "ImageExtensionHelper.h"

class CImageEx;

class CImage_DXTC
{
    // Typedefs
public:
protected:
    //////////////////////////////////////////////////////////////////////////
    // Extracted from Compressorlib.h on SDKs\CompressATI directory.
    // Added here because we are not really using the elements inside
    // this header file apart from those definitions as we are currently
    // loading the DLL CompressATI2.dll manually as recommended by the rendering
    // team.
    typedef enum
    {
        FORMAT_ARGB_8888,
        FORMAT_ARGB_TOOBIG
    } UNCOMPRESSED_FORMAT;

    typedef enum
    {
        COMPRESSOR_ERROR_NONE,
        COMPRESSOR_ERROR_NO_INPUT_DATA,
        COMPRESSOR_ERROR_NO_OUTPUT_POINTER,
        COMPRESSOR_ERROR_UNSUPPORTED_SOURCE_FORMAT,
        COMPRESSOR_ERROR_UNSUPPORTED_DESTINATION_FORMAT,
        COMPRESSOR_ERROR_UNABLE_TO_INIT_CODEC,
        COMPRESSOR_ERROR_GENERIC
    } COMPRESSOR_ERROR;
    //////////////////////////////////////////////////////////////////////////

    // Methods
public:
    CImage_DXTC();
    ~CImage_DXTC();

    // Arguments:
    //   pQualityLoss - 0 if info is not needed, pointer to the result otherwise - not need to preinitialize
    bool Load(const char* filename, CImageEx& outImage, bool* pQualityLoss = 0);        // true if success

    static inline const char* NameForTextureFormat(ETEX_Format ETF) { return CImageExtensionHelper::NameForTextureFormat(ETF); }
    static inline bool IsBlockCompressed(ETEX_Format ETF) { return CImageExtensionHelper::IsBlockCompressed(ETF); }
    static inline bool IsLimitedHDR(ETEX_Format ETF) { return CImageExtensionHelper::IsRangeless(ETF); }

    int TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF);

private:
    static COMPRESSOR_ERROR CheckParameters(
        int width,
        int height,
        UNCOMPRESSED_FORMAT destinationFormat,
        const void* sourceData,
        void* destinationData,
        int destinationDataSize);

    static COMPRESSOR_ERROR DecompressTextureBTC(
        int width,
        int height,
        ETEX_Format sourceFormat,
        UNCOMPRESSED_FORMAT destinationFormat,
        const int imageFlags,
        const void* sourceData,
        void* destinationData,
        int destinationDataSize,
        int destinationPageOffset);
};

#endif // CRYINCLUDE_EDITOR_UTIL_IMAGE_DXTC_H
