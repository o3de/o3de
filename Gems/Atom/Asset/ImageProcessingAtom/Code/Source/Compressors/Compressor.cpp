/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageProcessing_precompiled.h>

#include <Compressors/CTSquisher.h>
#include <Compressors/PVRTC.h>
#include <Compressors/ETC2.h>

// this is required for the AZ_TRAIT_IMAGEPROCESSING_USE_ISPC_TEXTURE_COMPRESSOR define
#include <ImageProcessing_Traits_Platform.h>

#if AZ_TRAIT_IMAGEPROCESSING_USE_ISPC_TEXTURE_COMPRESSOR
#include <Compressors/ISPCTextureCompressor.h>
#endif

namespace ImageProcessingAtom
{
    ICompressorPtr ICompressor::FindCompressor(EPixelFormat fmt, ColorSpace colorSpace, bool isCompressing)
    {
        // The ISPC texture compressor is able to compress BC1, BC3, BC6H and BC7 formats, and all of the ASTC formats.
        // Note: The ISPC texture compressor is only able to compress images that are a multiple of the compressed format's blocksize.
        // Another limitation is that the compressor requires LDR source images to be in sRGB colorspace.
#if AZ_TRAIT_IMAGEPROCESSING_USE_ISPC_TEXTURE_COMPRESSOR
        if (ISPCCompressor::IsCompressedPixelFormatSupported(fmt))
        {
            if ((isCompressing && ISPCCompressor::IsSourceColorSpaceSupported(colorSpace, fmt)) || (!isCompressing && ISPCCompressor::DoesSupportDecompress(fmt)))
            {
                return ICompressorPtr(new ISPCCompressor());
            }
        }
#endif

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
}; // namespace ImageProcessingAtom
