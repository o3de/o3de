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

#pragma once

#include <Compressors/Compressor.h>

struct bc6h_enc_settings;
struct bc7_enc_settings;
struct astc_enc_settings;

namespace ImageProcessingAtom
{
    // ISPC Texture Compressor 
    class ISPCCompressor
        : public ICompressor
    {
    public:
        static bool IsCompressedPixelFormatSupported(EPixelFormat fmt);
        static bool IsUncompressedPixelFormatSupported(EPixelFormat fmt);
        static bool DoesSupportDecompress(EPixelFormat fmtDst);
        static bool IsSourceColorSpaceSupported(ColorSpace colorSpace, EPixelFormat destinationFormat);

        ColorSpace GetSupportedColorSpace(EPixelFormat compressFormat) const final;
        IImageObjectPtr CompressImage(IImageObjectPtr sourceImage, EPixelFormat destinationFormat, const CompressOption* compressOption) const final;
        IImageObjectPtr DecompressImage(IImageObjectPtr sourceImage, EPixelFormat destinationFormat) const final;

        EPixelFormat GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) const final;
    };
}; // namespace ImageProcessingAtom
