/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>

#include <Compressors/CTSquisher.h>

namespace ImageProcessingAtom
{
    struct CrySquisherCallbackUserData
    {
        IImageObjectPtr m_pImageObject;
        AZ::u8* m_dstMem;
        AZ::u32 m_dstOffset;
    };

    // callbacks for the CryTextureSquisher
    void CrySquisherOutputCallback(const CryTextureSquisher::CompressorParameters& compress, const void* data, AZ::u32 size, AZ::u32 oy, AZ::u32 ox)
    {
        CrySquisherCallbackUserData* const pUserData = (CrySquisherCallbackUserData*)compress.userPtr;

        AZ::u32 stride = (compress.width + 3) >> 2;
        AZ::u32 blocks = (compress.height + 3) >> 2;
        memcpy(pUserData->m_dstMem + size * (stride * oy + ox), data, size);
        pUserData->m_dstOffset = size * (stride * blocks);
    }

    void CrySquisherInputCallback(const CryTextureSquisher::DecompressorParameters& decompress, void* data, AZ::u32 size, AZ::u32 oy, AZ::u32 ox)
    {
        CrySquisherCallbackUserData* const pUserData = (CrySquisherCallbackUserData*)decompress.userPtr;
        AZ::u32 stride = (decompress.width + 3) >> 2;
        AZ::u32 blocks = (decompress.height + 3) >> 2;

        //assert(CPixelFormats::GetPixelFormatInfo(pUserData->m_pImageObject->GetPixelFormat())->bCompressed);

        memcpy(data, pUserData->m_dstMem + size * (stride * oy + ox), size);

        pUserData->m_dstOffset = size * (stride * blocks);
    }


    CryTextureSquisher::ECodingPreset CTSquisher::GetCompressPreset(EPixelFormat compressFmt, EPixelFormat uncompressFmt)
    {
        CryTextureSquisher::ECodingPreset preset = CryTextureSquisher::eCompressorPreset_Num;
        switch (compressFmt)
        {
        case ePixelFormat_BC1:
            preset = CryTextureSquisher::eCompressorPreset_BC1U;
            break;
        case ePixelFormat_BC1a:
            preset = CryTextureSquisher::eCompressorPreset_BC1Ua;
            break;
        case ePixelFormat_BC3:
            preset = CryTextureSquisher::eCompressorPreset_BC3U;
            break;
        case ePixelFormat_BC3t:
            preset = CryTextureSquisher::eCompressorPreset_BC3Ut;
            break;
        case ePixelFormat_BC4:
            preset = (CPixelFormats::GetInstance().IsFormatSingleChannel(uncompressFmt)
                      ? CryTextureSquisher::eCompressorPreset_BC4Ua // a-channel
                      : CryTextureSquisher::eCompressorPreset_BC4U); // r-channel
            break;
        case ePixelFormat_BC4s:
            preset = (CPixelFormats::GetInstance().IsFormatSingleChannel(uncompressFmt)
                      ? CryTextureSquisher::eCompressorPreset_BC4Sa // a-channel
                      : CryTextureSquisher::eCompressorPreset_BC4S); // r-channel
            break;
        case ePixelFormat_BC5:
            preset = CryTextureSquisher::eCompressorPreset_BC5Un;
            break;
        case ePixelFormat_BC5s:
            preset = CryTextureSquisher::eCompressorPreset_BC5Sn;
            break;
        case ePixelFormat_BC6UH:
            preset = CryTextureSquisher::eCompressorPreset_BC6UH;
            break;
        case ePixelFormat_BC7:
            preset = CryTextureSquisher::eCompressorPreset_BC7U;
            break;
        case ePixelFormat_BC7t:
            preset = CryTextureSquisher::eCompressorPreset_BC7Ut;
            break;
        default:
            AZ_Assert(false, "%s: Unexpected pixel format (in compressing an image). Inform an RC programmer.", __FUNCTION__);
        }

        return preset;
    }

    bool CTSquisher::IsCompressedPixelFormatSupported(EPixelFormat fmt)
    {
        switch (fmt)
        {
        case ePixelFormat_BC1:
        case ePixelFormat_BC1a:
        case ePixelFormat_BC3:
        case ePixelFormat_BC3t:
        case ePixelFormat_BC4:
        case ePixelFormat_BC4s:
        case ePixelFormat_BC5:
        case ePixelFormat_BC5s:
        case ePixelFormat_BC6UH:
        case ePixelFormat_BC7:
        case ePixelFormat_BC7t:
            return true;
        default:
            return false;
        }
    }

    bool CTSquisher::IsUncompressedPixelFormatSupported(EPixelFormat fmt)
    {
        switch (fmt)
        {
        case ePixelFormat_R8:
        case ePixelFormat_A8:
        case ePixelFormat_R8G8B8A8:
        case ePixelFormat_R8G8B8X8:
        case ePixelFormat_R32F:
        case ePixelFormat_R32G32B32A32F:
            return true;
        default:
            return false;
        }
    }

    bool CTSquisher::DoesSupportDecompress([[maybe_unused]] EPixelFormat fmtDst)
    {
        return true;
    }

    ColorSpace CTSquisher::GetSupportedColorSpace([[maybe_unused]] EPixelFormat compressFormat) const
    {
        return ColorSpace::autoSelect;
    }    
        
    const char* CTSquisher::GetName() const
    {
        return "CTSquisher";
    }

    EPixelFormat CTSquisher::GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) const
    {
        //special cases
        if (compressedfmt == ePixelFormat_BC6UH || compressedfmt == ePixelFormat_BC5 || compressedfmt == ePixelFormat_BC5s)
        {
            return ePixelFormat_R32G32B32A32F;
        }

        if (IsUncompressedPixelFormatSupported(uncompressedfmt))
        {
            return uncompressedfmt;
        }

        //for fmt dont support, convert to supported uncompressed formats: ePixelFormat_A8, ePixelFormat_R8, ePixelFormat_A8R8G8B8,
        // ePixelFormat_X8R8G8B8, ePixelFormat_R32F, ePixelFormat_A32B32G32R32F

        switch (uncompressedfmt)
        {
        case ePixelFormat_R8G8:
        case ePixelFormat_R16G16:
        case ePixelFormat_R8G8B8:
        case ePixelFormat_B8G8R8:
            return ePixelFormat_R8G8B8X8;
        case ePixelFormat_R16:
            return ePixelFormat_R8;
        case ePixelFormat_R16G16B16A16:
        case ePixelFormat_B8G8R8A8:
            return ePixelFormat_R8G8B8A8;
        case ePixelFormat_R9G9B9E5:
        case ePixelFormat_R32G32F:
        case ePixelFormat_R16G16B16A16F:
        case ePixelFormat_R16G16F:
            return ePixelFormat_R32G32B32A32F;
        case ePixelFormat_R16F:
            return ePixelFormat_R32F;
        default:
            //this shouldn't happen. but we could handle it with uncompressed data anyway
            if (CPixelFormats::GetInstance().IsPixelFormatWithoutAlpha(uncompressedfmt))
            {
                return ePixelFormat_R8G8B8X8;
            }
            else
            {
                return ePixelFormat_R8G8B8A8;
            }
        }
    }

    IImageObjectPtr CTSquisher::DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) const
    {
        // Decompressing
        // the output pixel format could only have one channel or four channels. need to find out more
        EPixelFormat fmtSrc = srcImage->GetPixelFormat();

        //src format need to be compressed and dst format need to uncompressed.
        if (!IsCompressedPixelFormatSupported(fmtSrc) || !IsUncompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));

        //clear the dstImage to (0, 0, 0, 1) since some compression format only write to certain channels
        dstImage->ClearColor(0, 0, 0, 1);

        //for each mipmap
        const AZ::u32 mipCount = srcImage->GetMipCount();
        for (AZ::u32 dwMip = 0; dwMip < mipCount; ++dwMip)
        {
            const AZ::u32 dwLocalWidth = srcImage->GetWidth(dwMip);
            const AZ::u32 dwLocalHeight = srcImage->GetHeight(dwMip);

            AZ::u8* pSrcMem;
            AZ::u32 dwSrcPitch;
            srcImage->GetImagePointer(dwMip, pSrcMem, dwSrcPitch);

            AZ::u8* pDstMem;
            AZ::u32 dwDstPitch;
            dstImage->GetImagePointer(dwMip, pDstMem, dwDstPitch);

            CrySquisherCallbackUserData userData;
            userData.m_pImageObject = srcImage;
            userData.m_dstOffset = 0;
            userData.m_dstMem = pSrcMem;

            CryTextureSquisher::DecompressorParameters decompress;

            decompress.dstBuffer = pDstMem;
            decompress.width = dwLocalWidth;
            decompress.height = dwLocalHeight;
            decompress.pitch = dwDstPitch;

            decompress.dstType = (CPixelFormats::GetInstance().IsFormatFloatingPoint(fmtDst, true) ?
                                  CryTextureSquisher::eBufferType_ufloat : CryTextureSquisher::eBufferType_uint8);

            if (CPixelFormats::GetInstance().IsFormatSigned(fmtSrc))
            {
                decompress.dstType = (decompress.dstType == CryTextureSquisher::eBufferType_ufloat ?
                                      CryTextureSquisher::eBufferType_sfloat : CryTextureSquisher::eBufferType_sint8);
            }

            decompress.userPtr = &userData;
            decompress.userInputFunction = CrySquisherInputCallback;
            decompress.preset = GetCompressPreset(fmtSrc, fmtDst);

            CryTextureSquisher::Decompress(decompress);
        }

        // CTsquish operates on native normal vectors when floating-point
        // buffers are used. Apply bias and scale when returning a normal-map.
        if (fmtSrc == ePixelFormat_BC5 || fmtSrc == ePixelFormat_BC5s)
        {
            if (fmtDst == ePixelFormat_R32G32B32A32F)
            {
                //conver from [-1, 1] to [0, 1]. And set alpha to 1.
                dstImage->ScaleAndBiasChannels(0, 100,
                    AZ::Vector4(0.5f, 0.5f, 0.5f, 0.0f),
                    AZ::Vector4(0.5f, 0.5f, 0.5f, 1.0f));
            }
        }

        return dstImage;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    IImageObjectPtr CTSquisher::CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst,
        const CompressOption* compressOption) const
    {
        // Compressing
        EPixelFormat fmtSrc = srcImage->GetPixelFormat();

        //src format need to be uncompressed and dst format need to compressed.
        if (!IsUncompressedPixelFormatSupported(fmtSrc) || !IsCompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));

        //passing compress option
        ICompressor::EQuality quality = ICompressor::eQuality_Normal;
        AZ::Vector3 weights = AZ::Vector3(0.3333f, 0.3334f, 0.3333f);
        if (compressOption)
        {
            quality = compressOption->compressQuality;
            weights = compressOption->rgbWeight;
        }

        //do some clamp for float
        if (fmtSrc == ePixelFormat_R32G32B32A32F)
        {
            const uint32 nMips = srcImage->GetMipCount();

            // NOTES:
            // - all incoming images are unsigned, even normal maps
            // - all mipmaps of incoming images can contain out-of-range values from mipmap filtering
            // - 3Dc/BC5 is synonymous with "is a normal map" because they are not tagged explicitly as such
            if (fmtDst == ePixelFormat_BC5 || fmtDst == ePixelFormat_BC5s)
            {
                srcImage->ScaleAndBiasChannels(0, nMips,
                    AZ::Vector4(2.0f, 2.0f, 2.0f, 1.0f),
                    AZ::Vector4(-1.0f, -1.0f, -1.0f, 0.0f));
                srcImage->ClampChannels(0, nMips,
                    AZ::Vector4(-1.0f, -1.0f, -1.0f, -1.0f),
                    AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            else if (fmtDst == ePixelFormat_BC6UH)
            {
                srcImage->ClampChannels(0, nMips,
                    AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f),
                    AZ::Vector4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX));
            }
            else
            {
                srcImage->ClampChannels(0, nMips,
                    AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f),
                    AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            }
        }

        const uint32 mipCount = dstImage->GetMipCount();
        for (uint32 dwMip = 0; dwMip < mipCount; ++dwMip)
        {
            uint32 dwLocalWidth = srcImage->GetWidth(dwMip);
            uint32 dwLocalHeight = srcImage->GetHeight(dwMip);

            uint8* pSrcMem;
            uint32 dwSrcPitch;
            srcImage->GetImagePointer(dwMip, pSrcMem, dwSrcPitch);

            uint8* pDstMem;
            uint32 dwDstPitch;
            dstImage->GetImagePointer(dwMip, pDstMem, dwDstPitch);

            {
                CrySquisherCallbackUserData userData;
                userData.m_pImageObject = dstImage;
                userData.m_dstOffset = 0;
                userData.m_dstMem = pDstMem;

                CryTextureSquisher::CompressorParameters compress;

                compress.srcBuffer = pSrcMem;
                compress.width = dwLocalWidth;
                compress.height = dwLocalHeight;
                compress.pitch = dwSrcPitch;

                compress.srcType = (CPixelFormats::GetInstance().IsFormatFloatingPoint(fmtSrc, true) ?
                                    CryTextureSquisher::eBufferType_ufloat : CryTextureSquisher::eBufferType_uint8);
                if (CPixelFormats::GetInstance().IsFormatSigned(fmtDst))
                {
                    compress.srcType = (compress.srcType == CryTextureSquisher::eBufferType_ufloat ?
                                        CryTextureSquisher::eBufferType_sfloat : CryTextureSquisher::eBufferType_sint8);
                }

                const AZ::Vector3 uniform = AZ::Vector3(0.3333f, 0.3334f, 0.3333f);

                compress.weights[0] = weights.GetX();
                compress.weights[1] = weights.GetY();
                compress.weights[2] = weights.GetZ();

                compress.perceptual =
                    (compress.weights[0] != uniform.GetX()) ||
                    (compress.weights[1] != uniform.GetY()) ||
                    (compress.weights[2] != uniform.GetZ());

                compress.quality =
                    (quality == eQuality_Preview ? CryTextureSquisher::eQualityProfile_Low :
                     (quality == eQuality_Fast ? CryTextureSquisher::eQualityProfile_Low :
                      (quality == eQuality_Slow ? CryTextureSquisher::eQualityProfile_High :
                       CryTextureSquisher::eQualityProfile_Medium)));

                compress.userPtr = &userData;
                compress.userOutputFunction = CrySquisherOutputCallback;
                compress.preset = GetCompressPreset(fmtDst, fmtSrc);

                CryTextureSquisher::Compress(compress);
            }
        } // for: all mips

        return dstImage;
    }
}; //namespace ImageProcessingAtom
