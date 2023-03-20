/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        const char* GetName() const final;


        EPixelFormat GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) const final;
    };
}; // namespace ImageProcessingAtom
