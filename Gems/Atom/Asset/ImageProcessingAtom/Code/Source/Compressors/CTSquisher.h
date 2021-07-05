/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Compressors/CryTextureSquisher/CryTextureSquisher.h>
#include <Compressors/Compressor.h>

namespace ImageProcessingAtom
{
    //Cry Texture Squisher for all the BC compressions
    class CTSquisher
        : public ICompressor
    {
    public:
        static bool IsCompressedPixelFormatSupported(EPixelFormat fmt);
        static bool IsUncompressedPixelFormatSupported(EPixelFormat fmt);
        static bool DoesSupportDecompress(EPixelFormat fmtDst);

        IImageObjectPtr CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst, const CompressOption* compressOption) const override;
        IImageObjectPtr DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) const override;

        EPixelFormat GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) const override;
        ColorSpace GetSupportedColorSpace(EPixelFormat compressFormat) const final;

    private:
        static CryTextureSquisher::ECodingPreset GetCompressPreset(EPixelFormat compressFmt, EPixelFormat uncompressFmt);
    };
}; // namespace ImageProcessingAtom
