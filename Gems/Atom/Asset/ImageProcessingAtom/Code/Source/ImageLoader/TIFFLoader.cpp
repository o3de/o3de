/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>

#include <ImageLoader/ImageLoaders.h>
#include <Atom/ImageProcessing/ImageObject.h>

#include <QString>

#include <tiffio.h>  // TIFF library

namespace ImageProcessingAtom
{
    namespace TIFFLoader
    {
        class TiffFileRead
        {
        public:
            TiffFileRead(const AZStd::string& filename)
                : m_tif(nullptr)
            {
                m_tif = TIFFOpen(filename.c_str(), "r");
            }

            ~TiffFileRead()
            {
                if (m_tif != nullptr)
                {
                    TIFFClose(m_tif);
                }
            }

            TIFF* GetTiff()
            {
                return m_tif;
            }

        private:
            TIFF* m_tif;
        };

        bool IsExtensionSupported(const char* extension)
        {
            QString ext = QString(extension).toLower();
            // This is the list of file extensions supported by this loader
            return ext == "tif" || ext == "tiff";
        }

        // loads simple RAWImage from 8bit uint tiff source raster
        static IImageObject* Load8BitImageFromTIFF(TIFF* tif);
        // loads simple FloatImage from 16bit uint tiff source raster
        static IImageObject* Load16BitImageFromTIFF(TIFF* tif);
        // loads simple FloatImage from 16f HDR tiff source raster
        static IImageObject* Load16BitHDRImageFromTIFF(TIFF* tif);
        // loads simple FloatImage from 32f HDR tiff source raster
        static IImageObject* Load32BitHDRImageFromTIFF(TIFF* tif);

        IImageObject* LoadImageFromTIFF(const AZStd::string& filename)
        {
            TiffFileRead tiffRead(filename);
            TIFF* tif = tiffRead.GetTiff();

            IImageObject* pRet = nullptr;

            if (!tif)
            {
                AZ_Warning("Image Processing", false, "%s: Open tiff failed (%s)", __FUNCTION__, filename.c_str());
                return pRet;
            }

            uint32_t dwBitsPerChannel = 0;
            uint32_t dwChannels = 0;
            uint32_t dwFormat = 0;
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &dwChannels);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &dwBitsPerChannel);
            TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &dwFormat);

            if (dwChannels != 1 && dwChannels != 2 && dwChannels != 3 && dwChannels != 4)
            {
                AZ_Warning("Image Processing", false, "Unsupported TIFF pixel format (channel count: %d)", dwChannels);
                return pRet;
            }

            uint32_t dwWidth = 0;
            uint32_t dwHeight = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);
            if (dwWidth <= 0 || dwHeight <= 0)
            {
                AZ_Error("Image Processing", false, "%s failed (empty image)", __FUNCTION__);
                return pRet;
            }

            if (dwBitsPerChannel == 8)
            {
                // R8, GR8, BGR8, BGRA8
                pRet = Load8BitImageFromTIFF(tif);
            }
            else if (dwBitsPerChannel == 16)
            {
                // A/L/R16, R16F, GR16, GR16f, ARGB16, ARGB16f
                if (dwFormat == SAMPLEFORMAT_IEEEFP)
                {
                    pRet = Load16BitHDRImageFromTIFF(tif);
                }
                else
                {
                    pRet = Load16BitImageFromTIFF(tif);
                }
            }
            else if (dwBitsPerChannel == 32 && dwFormat == SAMPLEFORMAT_IEEEFP)
            {
                // A/L/R32f, GR32f, ARGB32f
                pRet = Load32BitHDRImageFromTIFF(tif);
            }
            else
            {
                AZ_Error("Image Processing", false, "File %s has unsupported TIFF pixel format. sample channels: %d,\
                    bits per channel: %d, sample format: %d", filename.c_str(), dwChannels, dwBitsPerChannel, dwFormat);
                return pRet;
            }

            if (pRet == nullptr)
            {
                AZ_Error("Image Processing", false, "Failed to read TIFF pixels");
                return pRet;
            }

            return pRet;
        }

        static IImageObject* Load8BitImageFromTIFF(TIFF* tif)
        {
            uint32_t dwChannels = 0;
            uint32_t dwPhotometric = 0;
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &dwChannels);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &dwPhotometric);

            uint32_t dwWidth = 0;
            uint32_t dwHeight = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);

            EPixelFormat eFormat = ePixelFormat_R8G8B8X8;

            if (dwChannels == 1 && dwPhotometric != PHOTOMETRIC_MINISBLACK)
            {
                eFormat = ePixelFormat_R8;
            }
            else if (dwChannels == 4)
            {
                eFormat = ePixelFormat_R8G8B8A8;
            }
            IImageObject* pRet = IImageObject::CreateImage(dwWidth, dwHeight, 1, eFormat);

            uint8_t* dst;
            uint32_t dwPitch;
            pRet->GetImagePointer(0, dst, dwPitch);

            AZStd::vector<uint8_t> buf(dwPitch);

            for (uint32_t dwY = 0; dwY < dwHeight; ++dwY)
            {
                TIFFReadScanline(tif, &buf[0], dwY, 0); // read raw row

                const uint8_t* srcLine = &buf[0];
                uint8_t* dstLine = &dst[dwPitch * dwY];

                if (dwChannels == 1)
                {
                    if (dwPhotometric != PHOTOMETRIC_MINISBLACK)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            dstLine[0] = srcLine[0];

                            dstLine += 1;
                            srcLine += dwChannels;
                        }
                    }
                    else
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            dstLine[0] = srcLine[0];
                            dstLine[1] = srcLine[0];
                            dstLine[2] = srcLine[0];
                            dstLine[3] = 0xFF;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else if (dwChannels == 2)
                {
                    if (dwPhotometric == PHOTOMETRIC_SEPARATED)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            // convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                            dstLine[0] = 0xFF - srcLine[0];
                            dstLine[1] = 0xFF - srcLine[1];
                            dstLine[2] = 0x00;
                            dstLine[3] = 0xFF;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else
                {
                    for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                    {
                        dstLine[0] = srcLine[0];
                        dstLine[1] = srcLine[1];
                        dstLine[2] = srcLine[2];
                        dstLine[3] = (dwChannels == 3) ? 0xFF : srcLine[3];

                        dstLine += 4;
                        srcLine += dwChannels;
                    }
                }
            }

            return pRet;
        }

        static IImageObject* Load16BitImageFromTIFF(TIFF* tif)
        {
            uint32_t dwChannels = 0;
            uint32_t dwPhotometric = 0;
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &dwChannels);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &dwPhotometric);

            uint32_t dwWidth = 0;
            uint32_t dwHeight = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);

            EPixelFormat eFormat = ePixelFormat_R16G16B16A16;

            if (dwChannels == 1 && dwPhotometric != PHOTOMETRIC_MINISBLACK)
            {
                eFormat = ePixelFormat_R16;
            }
            IImageObject* pRet = IImageObject::CreateImage(dwWidth, dwHeight, 1, eFormat);

            uint8_t* dst;
            uint32_t dwPitch;
            pRet->GetImagePointer(0, dst, dwPitch);

            AZStd::vector<char> buf(dwPitch);

            for (uint32_t dwY = 0; dwY < dwHeight; ++dwY)
            {
                TIFFReadScanline(tif, &buf[0], dwY, 0); // read raw row

                const uint16_t* srcLine = (const uint16_t*)&buf[0];
                uint16_t* dstLine = (uint16_t*)&dst[dwPitch * dwY];

                if (dwChannels == 1)
                {
                    if (dwPhotometric != PHOTOMETRIC_MINISBLACK)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            dstLine[0] = srcLine[0];

                            dstLine += 1;
                            srcLine += dwChannels;
                        }
                    }
                    else
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            dstLine[0] = srcLine[0];
                            dstLine[1] = srcLine[0];
                            dstLine[2] = srcLine[0];
                            dstLine[3] = 0xFFFF;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else if (dwChannels == 2)
                {
                    if (dwPhotometric == PHOTOMETRIC_SEPARATED)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            //convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                            dstLine[0] = 0xFFFF - srcLine[0];
                            dstLine[1] = 0xFFFF - srcLine[1];
                            dstLine[2] = 0x0000;
                            dstLine[3] = 0xFFFF;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else
                {
                    for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                    {
                        dstLine[0] = srcLine[0];
                        dstLine[1] = srcLine[1];
                        dstLine[2] = srcLine[2];
                        dstLine[3] = (dwChannels == 3) ? 0xFFFF : srcLine[3];

                        dstLine += 4;
                        srcLine += dwChannels;
                    }
                }
            }

            return pRet;
        }

        static IImageObject* Load16BitHDRImageFromTIFF(TIFF* tif)
        {
            uint32_t dwChannels = 0;
            uint32_t dwPhotometric = 0;
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &dwChannels);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &dwPhotometric);

            uint32_t dwWidth = 0;
            uint32_t dwHeight = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);

            EPixelFormat eFormat = ePixelFormat_R16G16B16A16F;

            if (dwChannels == 1 && dwPhotometric != PHOTOMETRIC_MINISBLACK)
            {
                eFormat = ePixelFormat_R16F;
            }

            IImageObject* pRet = IImageObject::CreateImage(dwWidth, dwHeight, 1, eFormat);

            uint8_t* dst;
            uint32_t dwPitch;
            pRet->GetImagePointer(0, dst, dwPitch);

            AZStd::vector<char> buf(dwPitch);

            static const uint16_t zero = 0;
            static const uint16_t one = 1;

            for (uint32_t dwY = 0; dwY < dwHeight; ++dwY)
            {
                TIFFReadScanline(tif, &buf[0], dwY, 0); // read raw row

                const uint16_t* srcLine = (const uint16_t*)&buf[0];
                uint16_t* dstLine = (uint16_t*)&dst[dwPitch * dwY];

                if (dwChannels == 1)
                {
                    if (dwPhotometric != PHOTOMETRIC_MINISBLACK)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            dstLine[0] = srcLine[0];

                            dstLine += 1;
                            srcLine += dwChannels;
                        }
                    }
                    else
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            dstLine[0] = srcLine[0];
                            dstLine[1] = srcLine[0];
                            dstLine[2] = srcLine[0];
                            dstLine[3] = one;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else if (dwChannels == 2)
                {
                    if (dwPhotometric == PHOTOMETRIC_SEPARATED)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            //but convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                            dstLine[0] = uint16_t(1.0f - srcLine[0]);
                            dstLine[1] = uint16_t(1.0f - srcLine[1]);
                            dstLine[2] = zero;
                            dstLine[3] = one;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else
                {
                    for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                    {
                        dstLine[0] = srcLine[0];
                        dstLine[1] = srcLine[1];
                        dstLine[2] = srcLine[2];
                        dstLine[3] = (dwChannels == 3) ? one : srcLine[3];

                        dstLine += 4;
                        srcLine += dwChannels;
                    }
                }
            }

            return pRet;
        }

        static IImageObject* Load32BitHDRImageFromTIFF(TIFF* tif)
        {
            uint32_t dwChannels = 0;
            uint32_t dwPhotometric = 0;
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &dwChannels);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &dwPhotometric);

            uint32_t dwWidth = 0;
            uint32_t dwHeight = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);

            EPixelFormat eFormat = ePixelFormat_R32G32B32A32F;

            if (dwChannels == 1 && dwPhotometric != PHOTOMETRIC_MINISBLACK)
            {
                eFormat = ePixelFormat_R32F;
            }

            IImageObject* pRet = IImageObject::CreateImage(dwWidth, dwHeight, 1, eFormat);

            uint8_t* dst;
            uint32_t dwPitch;
            pRet->GetImagePointer(0, dst, dwPitch);

            AZStd::vector<char> buf(dwPitch);

            for (uint32_t dwY = 0; dwY < dwHeight; ++dwY)
            {
                TIFFReadScanline(tif, &buf[0], dwY, 0); // read raw row

                const float* srcLine = (const float*)&buf[0];
                float* dstLine = (float*)&dst[dwPitch * dwY];

                if (dwChannels == 1)
                {
                    if (dwPhotometric != PHOTOMETRIC_MINISBLACK)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            // clamp negative values
                            const float v = AZ::GetMax(srcLine[0], 0.0f);
                            dstLine[0] = v;

                            ++dstLine;
                            ++srcLine;
                        }
                    }
                    else
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            // clamp negative values
                            const float v = AZ::GetMax(srcLine[0], 0.0f);
                            dstLine[0] = v;
                            dstLine[1] = v;
                            dstLine[2] = v;
                            dstLine[3] = 1.0f;

                            dstLine += 4;
                            ++srcLine;
                        }
                    }
                }
                else if (dwChannels == 2)
                {
                    if (dwPhotometric == PHOTOMETRIC_SEPARATED)
                    {
                        for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            //convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                            dstLine[0] = 1.0f - AZ::GetMax(srcLine[0], 0.0f);
                            dstLine[1] = 1.0f - AZ::GetMax(srcLine[1], 0.0f);
                            dstLine[2] = 0.0f;
                            dstLine[3] = 1.0f;

                            dstLine += 4;
                            srcLine += dwChannels;
                        }
                    }
                }
                else
                {
                    for (uint32_t dwX = 0; dwX < dwWidth; ++dwX)
                    {
                        // clamp negative values; don't swap red and blue -> RGB(A)
                        dstLine[0] = AZ::GetMax(srcLine[0], 0.0f);
                        dstLine[1] = AZ::GetMax(srcLine[1], 0.0f);
                        dstLine[2] = AZ::GetMax(srcLine[2], 0.0f);
                        dstLine[3] = (dwChannels == 3) ? 1.0f : AZ::GetMax(srcLine[3], 0.0f);

                        dstLine += 4;
                        srcLine += dwChannels;
                    }
                }
            }

            return pRet;
        }

        const AZStd::string LoadSettingFromTIFF(const AZStd::string& filename)
        {
            AZStd::string setting = "";

            TiffFileRead tiffRead(filename);
            TIFF* tif = tiffRead.GetTiff();

            if (tif == nullptr)
            {
                return setting;
            }

            // get image metadata
            const unsigned char* buffer = nullptr;
            unsigned int bufferLength = 0;

            if (!TIFFGetField(tif, TIFFTAG_PHOTOSHOP, &bufferLength, &buffer))  // 34377 IPTC TAG
            {
                return setting;
            }

            const unsigned char* const bufferEnd = buffer + bufferLength;

            // detailed structure here:
            // https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_pgfId-1037504
            while (buffer < bufferEnd)
            {
                const unsigned char* const bufferStart = buffer;

                // sanity check
                if (buffer[0] != '8' || buffer[1] != 'B' || buffer[2] != 'I' || buffer[3] != 'M')
                {
                    AZ_Warning("Image Processing", false, "Invalid Photoshop TIFF file [%s]!", filename.c_str());
                    return setting;
                }
                buffer += 4;

                // get image resource id
                const unsigned short resourceId = (((unsigned short)buffer[0]) << 8) | (unsigned short)buffer[1];
                buffer += 2;

                // get size of pascal string
                const unsigned int dwNameSize = (unsigned int)buffer[0];
                ++buffer;

                // get pascal string
                AZStd::string szName(buffer, buffer + dwNameSize);
                buffer += dwNameSize;

                // align 2 bytes
                if ((buffer - bufferStart) & 1)
                {
                    ++buffer;
                }

                // get size of resource data
                const unsigned int dwSize =
                    (((unsigned int)buffer[0]) << 24) |
                    (((unsigned int)buffer[1]) << 16) |
                    (((unsigned int)buffer[2]) << 8) |
                    (unsigned int)buffer[3];
                buffer += 4;

                // IPTC-NAA record. Contains the [File Info...] information. Old RC use this section to store the setting string.
                if (resourceId == 0x0404)
                {
                    const unsigned char* const iptcBufferStart = buffer;

                    // Old RC uses IPTC ApplicationRecord tags SpecialInstructions to store the setting string
                    // IPTC Details: https://iptc.org/std/photometadata/specification/mapping/iptc-pmd-newsmlg2.html
                    unsigned int iptcPos = 0;
                    while (iptcPos + 5 < dwSize)
                    {
                        int marker = iptcBufferStart[iptcPos++];
                        int recordNumber = iptcBufferStart[iptcPos++];
                        int dataSetNumber = iptcBufferStart[iptcPos++];
                        int fieldLength = (iptcBufferStart[iptcPos++] << 8);
                        fieldLength += iptcBufferStart[iptcPos++];

                        // Ignore fields other than SpecialInstructions
                        if (marker != 0x1C || recordNumber != 0x02 || dataSetNumber != 0x28)
                        {
                            iptcPos += fieldLength;
                            continue;
                        }

                        //save the setting string before close file
                        setting = AZStd::string(iptcBufferStart + iptcPos, iptcBufferStart + iptcPos + fieldLength);
                        return setting;
                    }
                }

                buffer += dwSize;

                // align 2 bytes
                if ((buffer - bufferStart) & 1)
                {
                    ++buffer;
                }
            }

            return setting;
        }
    }// namespace ImageTIFF
} //namespace ImageProcessingAtom
