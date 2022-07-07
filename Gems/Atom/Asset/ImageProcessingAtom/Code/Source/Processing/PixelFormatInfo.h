/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Processing/AzDXGIFormat.h>            // DX10+ formats. DXGI_FORMAT

#include <Atom/ImageProcessing/PixelFormats.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>

#include <ImageBuilderBaseType.h>

namespace ImageProcessingAtom
{
    //The original implementation was from cryhalf's CryConvertFloatToHalf and CryConvertHalfToFloat function
    struct SHalf
    {
        explicit SHalf(float floatValue)
        {
            AZ::u32 Result;

            AZ::u32 intValue = ((AZ::u32*)(&floatValue))[0];
            AZ::u32 Sign = (intValue & 0x80000000U) >> 16U;
            intValue = intValue & 0x7FFFFFFFU;

            if (intValue > 0x47FFEFFFU)
            {
                // The number is too large to be represented as a half.  Saturate to infinity.
                Result = 0x7FFFU;
            }
            else
            {
                if (intValue < 0x38800000U)
                {
                    // The number is too small to be represented as a normalized half.
                    // Convert it to a denormalized value.
                    AZ::u32 Shift = 113U - (intValue >> 23U);
                    intValue = (0x800000U | (intValue & 0x7FFFFFU)) >> Shift;
                }
                else
                {
                    // Rebias the exponent to represent the value as a normalized half.
                    intValue += 0xC8000000U;
                }

                Result = ((intValue + 0x0FFFU + ((intValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
            }
            h = static_cast<AZ::u16>(Result | Sign);
        }

        operator float() const
        {
            AZ::u32 Mantissa;
            AZ::u32 Exponent;
            AZ::u32 Result;

            Mantissa = h & 0x03FF;

            if ((h & 0x7C00) != 0)  // The value is normalized
            {
                Exponent = ((h >> 10) & 0x1F);
            }
            else if (Mantissa != 0)     // The value is denormalized
            {
                // Normalize the value in the resulting float
                Exponent = 1;

                do
                {
                    Exponent--;
                    Mantissa <<= 1;
                } while ((Mantissa & 0x0400) == 0);

                Mantissa &= 0x03FF;
            }
            else                        // The value is zero
            {
                Exponent = static_cast<AZ::u32>(-112);
            }

            Result = ((h & 0x8000) << 16) | // Sign
                ((Exponent + 112) << 23) | // Exponent
                (Mantissa << 13);          // Mantissa

            return *(float*)&Result;
        }

    private:
        AZ::u16 h;
    };

    enum class ESampleType
    {
        eSampleType_Uint8,
        eSampleType_Uint16,
        eSampleType_Uint32,
        eSampleType_Half,
        eSampleType_Float,
        eSampleType_Compressed,
    };

    struct PixelFormatInfo
    {
        uint32_t    nChannels;      // channel count per pixel
        bool        bHasAlpha;      // has alpha channel or not
        const char* szAlpha;        // a string of bits of alpha channel used to show brief of the pixel format
        uint32_t    minWidth;       // minimum width required for image using this pixel format
        uint32_t    minHeight;      // minimum height required for image using this pixel format
        uint32_t    blockWidth;     // width of the block for block based compressing
        uint32_t    blockHeight;    // Height of the block for block based compressing
        uint32_t    bitsPerBlock;   // bits per pixel before uncompressed
        bool        bSquarePow2;    // whether the pixel format requires image size be square and power of 2.
        DXGI_FORMAT d3d10Format;    // the mapping d3d10 pixel format
        ESampleType eSampleType;    // the data type used to present pixel
        const char* szName;         // name for showing in editors
        const char* szDescription;  // description for showing in editors
        bool        bCompressed;    // if it's a compressed format
        bool        bSelectable;    // shows up in the list of usable destination pixel formats in the dialog window
        uint32_t    fourCC;         // fourCC to identify a none d3d10 format

        PixelFormatInfo()
            : szAlpha(0)
            , bitsPerBlock(0)
            , d3d10Format(DXGI_FORMAT_UNKNOWN)
            , szName(0)
            , szDescription(0)
            , fourCC(0)
        {
        }

        PixelFormatInfo(
            uint32_t    a_bitsPerPixel,
            uint32_t    a_Channels,
            bool        a_Alpha,
            const char* a_szAlpha,
            uint32_t    a_minWidth,
            uint32_t    a_minHeight,
            uint32_t    a_blockWidth,
            uint32_t    a_blockHeight,
            uint32_t    a_bitsPerBlock,
            bool        a_bSquarePow2,
            DXGI_FORMAT a_d3d10Format,
            uint32_t    a_fourCC,
            ESampleType a_eSampleType,
            const char* a_szName,
            const char* a_szDescription,
            bool        a_bCompressed,
            bool        a_bSelectable);
    };

    class CPixelFormats
    {
    public:
        //singleton
        static CPixelFormats& GetInstance();
        static void DestroyInstance();

        const PixelFormatInfo* GetPixelFormatInfo(EPixelFormat format);

        bool IsPixelFormatWithoutAlpha(EPixelFormat format);
        bool IsPixelFormatUncompressed(EPixelFormat format);

        //functions seems only used for BC compressions. need re-evaluate later
        bool IsFormatSingleChannel(EPixelFormat fmt);
        bool IsFormatSigned(EPixelFormat fmt);
        bool IsFormatFloatingPoint(EPixelFormat fmt, bool bFullPrecision);

        //find pixel format by its name
        EPixelFormat FindPixelFormatByName(const char* name);

        //returns maximum lod levels for image which has certain pixel format, width and height.
        uint32 ComputeMaxMipCount(EPixelFormat format, uint32 imageWidth, uint32 imageHeight);

        //check if the input image size work with the pixel format. Some compression formats have requirements with the input image size.
        bool IsImageSizeValid(EPixelFormat format, uint32 imageWidth, uint32 imageHeight, bool logWarning);

        //get suitable new size for an image with certain width, height and pixel format
        void GetSuitableImageSize(EPixelFormat format, AZ::u32 imageWidth, AZ::u32 imageHeight,
            AZ::u32& outWidth, AZ::u32& outHeight);

        //check if the image size of the specified pixel format need to be integer mutiple of block size
        bool CanImageSizeIgnoreBlockSize(EPixelFormat format);

        //eavluate image data size. it doesn't include mips
        uint32 EvaluateImageDataSize(EPixelFormat format, uint32 imageWidth, uint32 imageHeight);

    private:
        CPixelFormats();
        void InitPixelFormats();
        void InitPixelFormat(EPixelFormat format, const PixelFormatInfo& formatInfo);

    private:
        static CPixelFormats* s_instance;

        PixelFormatInfo m_pixelFormatInfo[ePixelFormat_Count];

        //pixel format name to pixel format enum
        AZStd::map<AZStd::string, EPixelFormat>  m_pixelFormatNameMap;
    };

    template <class TInteger>
    bool IsPowerOfTwo(TInteger x);
}  // namespace ImageProcessingAtom
