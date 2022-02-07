/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/std/function/function_template.h>

#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>

#include <Compressors/ISPCTextureCompressor.h>

#include <ISPC/ispc_texcomp.h>

namespace ImageProcessingAtom
{
    // Class used to store functions to specific quality profiles.
    class CompressionProfile
    {
    public:
        AZStd::function<void(bc6h_enc_settings*)> GetBC6() const
        {
            return m_bc6;
        }

        AZStd::function<void(bc7_enc_settings*)> GetBC7(bool discardAlpha) const
        {
            if (discardAlpha)
            {
                return m_bc7;
            }

            return m_bc7Alpha;
        }

        AZStd::function<void(astc_enc_settings*, int block_width, int block_height)> GetASTC(bool discardAlpha) const
        {
            if (discardAlpha)
            {
                return m_astc;
            }

            return m_astcAlpha;
        }

        AZStd::function<void(bc6h_enc_settings*)>   m_bc6;
        AZStd::function<void(bc7_enc_settings*)>    m_bc7;
        AZStd::function<void(bc7_enc_settings*)>    m_bc7Alpha;
        AZStd::function<void(astc_enc_settings*, int block_width, int block_height)>   m_astc;
        AZStd::function<void(astc_enc_settings*, int block_width, int block_height)>   m_astcAlpha;
    };

    bool ISPCCompressor::IsCompressedPixelFormatSupported(EPixelFormat fmt)
    {
        // Even though the ISPC compressor support ASTC formats. But it has restrictions
        //      1. Only supports LDR color profile
        //      2. Only supports a subset of 2D block sizes
        // Also it has overall lower quality compare to astc-encoder
        // So we won't add ASTC as part of supported formats here
        // Ref: https://solidpixel.github.io/2020/03/02/astc-compared.html
        switch (fmt)
        {
        case ePixelFormat_BC3:
        case ePixelFormat_BC6UH:
        case ePixelFormat_BC7:
        case ePixelFormat_BC7t:
            return true;
        default:
            return false;
        }
    }

    bool ISPCCompressor::IsUncompressedPixelFormatSupported(EPixelFormat fmt)
    {
        return (fmt == ePixelFormat_R16G16B16A16F || fmt == ePixelFormat_R8G8B8A8);
    }

    bool ISPCCompressor::DoesSupportDecompress([[maybe_unused]] EPixelFormat fmtDst)
    {
        return false;
    }

    bool ISPCCompressor::IsSourceColorSpaceSupported(ColorSpace colorSpace, EPixelFormat destinationFormat)
    {
        switch (destinationFormat)
        {
        case ePixelFormat_BC3:
            return !(colorSpace == ColorSpace::linear);
        case ePixelFormat_BC6UH:
        case ePixelFormat_BC7:
        case ePixelFormat_BC7t:
            return true;
        }

        AZ_Warning("ISPC Texture Compressor", false, "Destination format is not supported");
        return false;
    }

    ColorSpace ISPCCompressor::GetSupportedColorSpace(EPixelFormat compressFormat) const
    {
        switch (compressFormat)
        {
        case ePixelFormat_BC3:
            return ColorSpace::sRGB;
        case ePixelFormat_BC6UH:
        case ePixelFormat_BC7:
        case ePixelFormat_BC7t:
            return ColorSpace::autoSelect;
        }

        AZ_Warning("ISPC Texture Compressor", false, "Compression format is not supported.");
        return ColorSpace::autoSelect;
    }

    const char* ISPCCompressor::GetName() const
    {
        return "ISPCCompressor";
    }

    IImageObjectPtr ISPCCompressor::CompressImage(IImageObjectPtr sourceImage, EPixelFormat destinationFormat, const CompressOption* compressOption) const
    {
        // Used to find the profile setters, depending on the image quality
        using QualityTypeAndFunctionPair = AZStd::pair<ICompressor::EQuality, CompressionProfile>;
        const uint32_t QualityTypeCount = static_cast<uint32_t>(ICompressor::EQuality::Count);
        static const AZStd::array<QualityTypeAndFunctionPair, QualityTypeCount> qualityTypeMap =
        {
                AZStd::make_pair(ICompressor::eQuality_Preview, CompressionProfile {
                    GetProfile_bc6h_veryfast, GetProfile_ultrafast, GetProfile_alpha_ultrafast, GetProfile_astc_fast, GetProfile_astc_alpha_fast }),
                AZStd::make_pair(ICompressor::eQuality_Fast, CompressionProfile {
                    GetProfile_bc6h_fast, GetProfile_fast, GetProfile_alpha_fast, GetProfile_astc_fast, GetProfile_astc_alpha_fast }),
                AZStd::make_pair(ICompressor::eQuality_Normal, CompressionProfile {
                    GetProfile_bc6h_basic, GetProfile_basic, GetProfile_alpha_basic, GetProfile_astc_alpha_slow, GetProfile_astc_alpha_slow }),
                AZStd::make_pair(ICompressor::eQuality_Slow, CompressionProfile {
                    GetProfile_bc6h_basic, GetProfile_basic, GetProfile_alpha_basic, GetProfile_astc_alpha_slow, GetProfile_astc_alpha_slow })
        };
        static const CompressionProfile DefaultQuality = { GetProfile_bc6h_basic, GetProfile_basic, GetProfile_alpha_basic, GetProfile_astc_alpha_slow, GetProfile_astc_alpha_slow };

        EPixelFormat sourceFormat = sourceImage->GetPixelFormat();

        // Get suggested uncompressed format provides correspond uncompressed formats, 
        // this is just to validate format again here
        AZ_Assert(sourceFormat == GetSuggestedUncompressedFormat(destinationFormat, sourceFormat), "GetSuggestedUncompressedFormat need to be called to get the proper uncompress format as input");

        // Source format need to be uncompressed and destination format need to compressed.
        if (!IsUncompressedPixelFormatSupported(sourceFormat) || !IsCompressedPixelFormatSupported(destinationFormat))
        {
            return nullptr;
        }

        // Get quality setting and alpha setting
        ICompressor::EQuality quality = ICompressor::eQuality_Normal;
        bool discardAlpha = false;
        if (compressOption)
        {
            quality = compressOption->compressQuality;
            discardAlpha = compressOption->discardAlpha;
        }

        // Get the compression profile
        const CompressionProfile* compressionProfile = nullptr;
        {
            // Find the quality profile functions
            const auto qualityTypeMapIt = AZStd::find_if(qualityTypeMap.begin(), qualityTypeMap.end(), [quality](const QualityTypeAndFunctionPair& qualityTypeAndFunctionPair)
            {
                return qualityTypeAndFunctionPair.first == quality;
            });

            // Get the compression profile if it is available
            if (qualityTypeMapIt != qualityTypeMap.end())
            {
                compressionProfile = &qualityTypeMapIt->second;
            }
            else
            {   
                // Use the default profile if it doesn't exist
                compressionProfile = &DefaultQuality;
            }
        }

        // Allocate the destination image
        IImageObjectPtr destinationImage(sourceImage->AllocateImage(destinationFormat));

        // Compress the images per mip
        const uint32 mipCount = destinationImage->GetMipCount();
        for (uint32_t mip = 0; mip < mipCount; mip++)
        {
            // Create rgba_surface as input
            uint32 sourcePitch = 0;
            AZ::u8* sourceImageData = nullptr;
            sourceImage->GetImagePointer(mip, sourceImageData, sourcePitch);
            rgba_surface sourceSurface = {};
            {
                sourceSurface.ptr = sourceImageData;
                sourceSurface.width = sourceImage->GetWidth(mip);
                sourceSurface.height = sourceImage->GetHeight(mip);
                sourceSurface.stride = static_cast<int32_t>(sourcePitch);
            }

            // Get the mip image destination pointer
            uint32_t destinationPitch = 0;
            AZ::u8* destinationImageData = nullptr;
            destinationImage->GetImagePointer(mip, destinationImageData, destinationPitch);

            // Compress with the correct function, depending on the destination format
            switch (destinationFormat)
            {
            case ePixelFormat_BC3:
                CompressBlocksBC3(&sourceSurface, destinationImageData);
                break;
            case ePixelFormat_BC6UH:
            {
                // Get the profile setter
                bc6h_enc_settings settings = {};
                const auto setProfile = compressionProfile->GetBC6();
                setProfile(&settings);

                // Compress with BC6 half precision
                CompressBlocksBC6H(&sourceSurface, destinationImageData, &settings);
            }
            break;
            case ePixelFormat_BC7:
            case ePixelFormat_BC7t:
            {
                // Get the profile setter
                bc7_enc_settings settings = {};
                const auto setProfile = compressionProfile->GetBC7(discardAlpha);
                setProfile(&settings);

                // Compress with BC7
                CompressBlocksBC7(&sourceSurface, destinationImageData, &settings);
            }
            break;
            default:
            {
                // No valid pixel format
                AZ_Assert(false, "Unhandled pixel format %d", destinationFormat);
                return nullptr;
            }
            break;
            }
        }

        return destinationImage;
    }

    IImageObjectPtr ISPCCompressor::DecompressImage(IImageObjectPtr sourceImage, [[maybe_unused]] EPixelFormat destinationFormat) const
    {
        return nullptr;
    }

    EPixelFormat ISPCCompressor::GetSuggestedUncompressedFormat(EPixelFormat compressedfmt, [[maybe_unused]] EPixelFormat uncompressedfmt) const
    {
        if (compressedfmt == ePixelFormat_BC6UH)
        {
            return ePixelFormat_R16G16B16A16F;
        }

        return ePixelFormat_R8G8B8A8;
    }

}; // namespace ImageProcessingAtom
