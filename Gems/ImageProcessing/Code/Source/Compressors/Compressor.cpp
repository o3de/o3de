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

#include <ImageProcessing_precompiled.h>

#include <Compressors/CTSquisher.h>
#include <Compressors/PVRTC.h>
#include <Compressors/ETC2.h>

namespace ImageProcessing
{
    ICompressorPtr ICompressor::FindCompressor(EPixelFormat fmt, bool isCompressing)
    {
        if (CTSquisher::IsCompressedPixelFormatSupported(fmt))
        {
            if (isCompressing || (!isCompressing && CTSquisher::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new CTSquisher());
            }
        }

        // Both ETC2Compressor and PVRTCCompressor can process ETC formats
        // According to Mobile team, Etc2Com is faster than PVRTexLib, so we check with ETC2Compressor before PVRTCCompressor
        // Note: with the test I have done, I found out it cost similar time for both Etc2Com and PVRTexLib to compress
        // a 2048x2048 test texture to EAC_R11 and EAC_RG11. It was around 7 minutes for EAC_R11 and 14 minutes for EAC_RG11
        if (ETC2Compressor::IsCompressedPixelFormatSupported(fmt))
        {
            if (isCompressing || (!isCompressing && ETC2Compressor::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new ETC2Compressor());
            }
        }

        if (PVRTCCompressor::IsCompressedPixelFormatSupported(fmt))
        {
            if (isCompressing || (!isCompressing && PVRTCCompressor::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new PVRTCCompressor());
            }
        }

        return nullptr;
    }

    ICompressor::~ICompressor()
    {
        
    }
} // namespace ImageProcessing