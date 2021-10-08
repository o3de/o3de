/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuilderSettings/ImageProcessingDefines.h>
#include <Atom/ImageProcessing/PixelFormats.h>
#include <Atom/ImageProcessing/ImageObject.h>

namespace ImageProcessingAtom
{
    class ICompressor;
    typedef AZStd::shared_ptr<ICompressor> ICompressorPtr;

    //the interface base class for any compressors which can decompress/compress image with compressed pixel format
    class ICompressor
    {
    public:
        enum EQuality : uint32_t
        {
            eQuality_Preview = 0u,          // for the 256x256 preview only
            eQuality_Fast,
            eQuality_Normal,
            eQuality_Slow,

            Count
        };

        //some extra information required for different compressors.
        //keep is a simple structure for now.
        struct CompressOption
        {
            EQuality compressQuality = eQuality_Normal;
            //required for CTSquisher
            AZ::Vector3 rgbWeight = AZ::Vector3(0.3333f, 0.3334f, 0.3333f);
            bool discardAlpha = false;
        };

    public:
        //compress the source image to desired compressed pixel format
        virtual IImageObjectPtr CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst, const CompressOption* compressOption) const = 0;
        virtual IImageObjectPtr DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) const = 0;
        virtual EPixelFormat GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) const = 0;
        virtual ColorSpace GetSupportedColorSpace(EPixelFormat compressFormat) const = 0;
        virtual const char* GetName() const = 0;

        //find compressor for specified compressed pixel format. isCompressing to indicate if it's for compressing or decompressing
        static ICompressorPtr FindCompressor(EPixelFormat fmt, ColorSpace colorSpace, bool isCompressing);

        virtual ~ICompressor() = 0;
    };
}; // namespace ImageProcessingAtom
