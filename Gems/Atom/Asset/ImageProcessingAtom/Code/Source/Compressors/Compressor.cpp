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

        return nullptr;
    }

    ICompressor::~ICompressor()
    {
    }
}; // namespace ImageProcessingAtom
