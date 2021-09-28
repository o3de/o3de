/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformIncl.h>
#include <Compressors/ASTCCompressor.h>
#include <Compressors/CTSquisher.h>
#include <Compressors/PVRTC.h>
#include <Compressors/ETC2.h>
#include <Compressors/ISPCTextureCompressor.h>

namespace ImageProcessingAtom
{
    ICompressorPtr ICompressor::FindCompressor(EPixelFormat fmt, ColorSpace colorSpace, bool isCompressing)
    {
        if (ISPCCompressor::IsCompressedPixelFormatSupported(fmt))
        {
            if ((isCompressing && ISPCCompressor::IsSourceColorSpaceSupported(colorSpace, fmt)) || (!isCompressing && ISPCCompressor::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new ISPCCompressor());
            }
        }

        if (CTSquisher::IsCompressedPixelFormatSupported(fmt))
        {
            if (isCompressing || (!isCompressing && CTSquisher::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new CTSquisher());
            }
        }
        
        if (ASTCCompressor::IsCompressedPixelFormatSupported(fmt))
        {
            if (isCompressing || (!isCompressing && ASTCCompressor::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new ASTCCompressor());
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
}; // namespace ImageProcessingAtom
