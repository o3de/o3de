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

#include <ImageProcessing/PixelFormats.h>
#include <ImageProcessing/ImageObject.h>

namespace ImageProcessing
{
    class ICompressor;
    typedef AZStd::shared_ptr<ICompressor> ICompressorPtr;
    
    //the interface base class for any compressors which can decompress/compress image with compressed pixel format
    class ICompressor
    {
    public:
        enum EQuality
        {
            eQuality_Preview,          // for the 256x256 preview only
            eQuality_Fast,
            eQuality_Normal,
            eQuality_Slow,
        };

        //some extra information required for different compressors. 
        //keep is a simple structure for now. 
        struct CompressOption
        {
            EQuality compressQuality = eQuality_Normal;
            //required for CTSquisher
            AZ::Vector3 rgbWeight = AZ::Vector3(0.3333f, 0.3334f, 0.3333f);
        };

    public:
        //compress the source image to desired compressed pixel format
        virtual IImageObjectPtr CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst, const CompressOption *compressOption) = 0;
        virtual IImageObjectPtr DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) = 0;
        virtual EPixelFormat GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) = 0;

        //find compressor for specified compressed pixel format. isCompressing to indicate if it's for compressing or decompressing
        static ICompressorPtr FindCompressor(EPixelFormat fmt, bool isCompressing);
        
        virtual ~ICompressor() = 0;
    };

} // namespace ImageProcessing
