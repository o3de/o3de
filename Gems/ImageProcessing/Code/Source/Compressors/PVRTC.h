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

namespace ImageProcessing
{
    class PVRTCCompressor : public ICompressor
    {
    public:
        static bool IsCompressedPixelFormatSupported(EPixelFormat fmt);
        static bool IsUncompressedPixelFormatSupported(EPixelFormat fmt);
        static bool DoesSupportDecompress(EPixelFormat fmtDst);

        IImageObjectPtr CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst, const CompressOption *compressOption) override;
        IImageObjectPtr DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) override;

        EPixelFormat GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) override;
    };

} // namespace ImageProcessing