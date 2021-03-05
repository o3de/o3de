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

#include "ImageProcessing_precompiled.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Casting/numeric_cast.h>

#include <ImageLoader/ImageLoaders.h>
#include <ImageProcessing/ImageObject.h>

#include <QString>

#include <libtiff/tiffio.h>  // TIFF library

namespace ImageProcessing
{
    namespace TIFFLoader
    {
        class TiffFileRead
        {
        public:
            TiffFileRead(const AZStd::string& filename)
                : m_tif(nullptr)
            {
                m_tif = TIFFOpen(filename.c_str(), "r");;
            }

            ~TiffFileRead()
            {
                if (m_tif != nullptr)
                {
                    TIFFClose(m_tif);
                }
            }

            TIFF *GetTiff()
            {
                return m_tif;
            }

        private:
            TIFF *m_tif;
        };

        bool IsExtensionSupported(const char* extension)
        {
            QString ext = QString(extension).toLower();
            // This is the list of file extensions supported by this loader
            return ext == "tif" || ext == "tiff";
        }

        struct TiffData
        {
            AZ::u32 m_channels = 0;
            AZ::u32 m_photometric = 0;
            AZ::u32 m_bitsPerPixel = 0;
            AZ::u16 m_format = 0;

            AZ::u32 m_width = 0;
            AZ::u32 m_height = 0;

            AZ::u32 m_tileWidth = 0;
            AZ::u32 m_tileHeight = 0;
            bool m_isTiled = false;
            AZ::u32 m_bufSize = 0;

            bool m_isGeoTiff = false;
            float m_pixelValueScale = 1.0f;

            EPixelFormat m_pixelFormat = EPixelFormat::ePixelFormat_Unknown;
        };

        static void Process8BitTiff(AZ::u8* dst, const AZ::u8* src, AZ::u32 destIdx, AZ::u32 srcIdx, const TiffData& data, AZ::u8& dstMult)
        {
            if (data.m_channels == 1)
            {
                if (data.m_photometric != PHOTOMETRIC_MINISBLACK)
                {
                    dst[destIdx] = aznumeric_cast<AZ::u8>(src[srcIdx] * data.m_pixelValueScale);
                }
                else
                {
                    dst[destIdx] = aznumeric_cast<AZ::u8>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::u8>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 2] = aznumeric_cast<AZ::u8>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 3] = 0xFF;

                    dstMult = 4;
                }
            }
            else if (data.m_channels == 2)
            {
                if (data.m_photometric == PHOTOMETRIC_SEPARATED)
                {
                    // convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                    dst[destIdx] = aznumeric_cast<AZ::u8>(0xFF - src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::u8>(0xFF - src[srcIdx + 1] * data.m_pixelValueScale);
                    dst[destIdx + 2] = 0x00;
                    dst[destIdx + 3] = 0xFF;

                    dstMult = 4;
                }
                else
                {
                    dst[destIdx] = aznumeric_cast<AZ::u8>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::u8>(src[srcIdx + 1] * data.m_pixelValueScale);

                    dstMult = 2;
                }
            }
            else
            {
                dst[destIdx] = aznumeric_cast<AZ::u8>(src[srcIdx] * data.m_pixelValueScale);
                dst[destIdx + 1] = aznumeric_cast<AZ::u8>(src[srcIdx + 1] * data.m_pixelValueScale);
                dst[destIdx + 2] = aznumeric_cast<AZ::u8>(src[srcIdx + 2] * data.m_pixelValueScale);
                dst[destIdx + 3] = (data.m_channels == 3) ? 0xFF : aznumeric_cast<AZ::u8>(src[srcIdx + 3]  * data.m_pixelValueScale);

                dstMult = 4;
            }
        }

        static void Process16BitHDRTiff(AZ::s16* dst, const AZ::s16* src, AZ::u32 destIdx, AZ::u32 srcIdx, const TiffData& data, AZ::u8& dstMult)
        {
            if (data.m_channels == 1)
            {
                if (data.m_photometric != PHOTOMETRIC_MINISBLACK)
                {
                    dst[destIdx] = aznumeric_cast<AZ::s16>(src[srcIdx] * data.m_pixelValueScale);
                }
                else
                {
                    dst[destIdx] = aznumeric_cast<AZ::s16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::s16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 2] = aznumeric_cast<AZ::s16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 3] = 1;

                    dstMult = 4;
                }
            }
            else if (data.m_channels == 2)
            {
                if (data.m_photometric == PHOTOMETRIC_SEPARATED)
                {
                    //but convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                    dst[destIdx] = uint16(1.0f - src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = uint16(1.0f - src[srcIdx + 1] * data.m_pixelValueScale);
                    dst[destIdx + 2] = 0;
                    dst[destIdx + 3] = 1;

                    dstMult = 4;
                }
                else
                {
                    dst[destIdx] = aznumeric_cast<AZ::s16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::s16>(src[srcIdx + 1] * data.m_pixelValueScale);

                    dstMult = 2;
                }
            }
            else
            {
                dst[destIdx] = aznumeric_cast<AZ::s16>(src[srcIdx] * data.m_pixelValueScale);
                dst[destIdx + 1] = aznumeric_cast<AZ::s16>(src[srcIdx + 1] * data.m_pixelValueScale);
                dst[destIdx + 2] = aznumeric_cast<AZ::s16>(src[srcIdx + 2] * data.m_pixelValueScale);
                dst[destIdx + 3] = (data.m_channels == 3) ? 1 : aznumeric_cast<AZ::s16>(src[srcIdx + 3] * data.m_pixelValueScale);

                dstMult = 4;
            }
        }

        static void Process16BitTiff(AZ::u16* dst, const AZ::u16* src, AZ::u32 destIdx, AZ::u32 srcIdx, const TiffData& data, AZ::u8& dstMult)
        {
            if (data.m_channels == 1)
            {
                if (data.m_photometric != PHOTOMETRIC_MINISBLACK)
                {
                    dst[destIdx] = aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                }
                else
                {
                    dst[destIdx] = aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 2] = aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 3] = 0xFFFF;

                    dstMult = 4;
                }
            }
            else if (data.m_channels == 2)
            {
                if (data.m_photometric == PHOTOMETRIC_SEPARATED)
                {
                    //convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                    dst[destIdx] = 0xFFFF - aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = 0xFFFF - aznumeric_cast<AZ::u16>(src[srcIdx + 1] * data.m_pixelValueScale);
                    dst[destIdx + 2] = 0x0000;
                    dst[destIdx + 3] = 0xFFFF;

                    dstMult = 4;
                }
                else
                {
                    dst[destIdx] = aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                    dst[destIdx + 1] = aznumeric_cast<AZ::u16>(src[srcIdx + 1] * data.m_pixelValueScale);

                    dstMult = 2;
                }
            }
            else
            {
                dst[destIdx] = aznumeric_cast<AZ::u16>(src[srcIdx] * data.m_pixelValueScale);
                dst[destIdx + 1] = aznumeric_cast<AZ::u16>(src[srcIdx + 1] * data.m_pixelValueScale);
                dst[destIdx + 2] = aznumeric_cast<AZ::u16>(src[srcIdx + 2] * data.m_pixelValueScale);
                dst[destIdx + 3] = (data.m_channels == 3) ? 0xFFFF : aznumeric_cast<AZ::u16>(src[srcIdx + 3] * data.m_pixelValueScale);

                dstMult = 4;
            }
        }

        static void Process32BitHDRTiff(float* dst, const float* src, AZ::u32 destIdx, AZ::u32 srcIdx, const TiffData& data, AZ::u8& dstMult)
        {
            auto getScaledOrClamped = [&data](auto val)
            {
                //  GeoTiff doesn't clamp because negative values are legitimate when the data represents height values below sea level.
                return data.m_isGeoTiff ? (val * data.m_pixelValueScale) : AZ::GetMax(val, 0.0f);
            };

            if (data.m_channels == 1)
            {
                if (data.m_photometric != PHOTOMETRIC_MINISBLACK)
                {
                    // clamp negative values
                    const float v = getScaledOrClamped(src[srcIdx]);
                    dst[destIdx] = v;
                }
                else
                {
                    // clamp negative values
                    const float v = getScaledOrClamped(src[srcIdx]);
                    dst[destIdx] = v;
                    dst[destIdx + 1] = v;
                    dst[destIdx + 2] = v;
                    dst[destIdx + 3] = 1.0f;

                    dstMult = 4;
                }
            }
            else if (data.m_channels == 2)
            {
                if (data.m_photometric == PHOTOMETRIC_SEPARATED)
                {
                    //convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                    dst[destIdx] = 1.0f - getScaledOrClamped(src[srcIdx]);
                    dst[destIdx + 1] = 1.0f - getScaledOrClamped(src[srcIdx + 1]);
                    dst[destIdx + 2] = 0.0f;
                    dst[destIdx + 3] = 1.0f;

                    dstMult = 4;
                }
                else
                {
                    dst[destIdx] = src[srcIdx] * data.m_pixelValueScale;
                    dst[destIdx + 1] = src[srcIdx + 1] * data.m_pixelValueScale;

                    dstMult = 2;
                }
            }
            else
            {
                // clamp negative values; don't swap red and blue -> RGB(A)
                dst[destIdx] = getScaledOrClamped(src[srcIdx]);
                dst[destIdx + 1] = getScaledOrClamped(src[srcIdx + 1]);
                dst[destIdx + 2] = getScaledOrClamped(src[srcIdx + 2]);
                dst[destIdx + 3] = (data.m_channels == 3) ? 1.0f : getScaledOrClamped(src[srcIdx + 3]) * data.m_pixelValueScale;

                dstMult = 4;
            }
        }

        static TiffData GetTiffData(TIFF* tif)
        {
            TiffData data;

            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &data.m_channels);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &data.m_photometric);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &data.m_bitsPerPixel);
            TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &data.m_format);
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &data.m_width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &data.m_height);
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &data.m_tileWidth);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &data.m_tileHeight);

            // Check to see if this is a tiled TIFF (vs a scanline-based TIFF)
            if ((data.m_tileWidth > 0) && (data.m_tileHeight > 0))
            {
                // Tiled TIFF, so our buffer needs to be tile-sized
                data.m_isTiled = true;
                data.m_bufSize = TIFFTileSize(tif);
            }
            else
            {
                // Scanline TIFF, so our buffer needs to be scanline-sized.
                data.m_bufSize = TIFFScanlineSize(tif);

                // Treat scanlines like a tile of 1 x width size.
                data.m_tileHeight = 1;
                data.m_tileWidth = data.m_width;
            }

            // Defined in GeoTIFF format - http://web.archive.org/web/20160403164508/http://www.remotesensing.org/geotiff/spec/geotiffhome.html
            // Used to get the X, Y, Z scales from a GeoTIFF file
            constexpr auto GEOTIFF_MODELPIXELSCALE_TAG = 33550;

            // Check to see if it's a GeoTIFF, and if so, whether or not it has the ZScale parameter.
            AZ::u32 tagCount = 0;
            double* pixelScales = nullptr;
            if (TIFFGetField(tif, GEOTIFF_MODELPIXELSCALE_TAG, &tagCount, &pixelScales) == 1)
            {
                data.m_isGeoTiff = true;

                // if there's an xyz scale, and the Z scale isn't 0, let's use it.
                if ((tagCount == 3) && (pixelScales != nullptr) && (pixelScales[2] != 0.0f))
                {
                    data.m_pixelValueScale = static_cast<float>(pixelScales[2]);
                }
            }

            // Retrieve the pixel format of the image
            switch (data.m_bitsPerPixel)
            {
            case 8:
            {
                data.m_pixelFormat = ePixelFormat_R8G8B8X8;

                if (data.m_channels == 1 && data.m_photometric != PHOTOMETRIC_MINISBLACK)
                {
                    data.m_pixelFormat = ePixelFormat_R8;
                }
                else if (data.m_channels == 4)
                {
                    data.m_pixelFormat = ePixelFormat_R8G8B8A8;
                }

                break;
            }

            case 16:
            {
                if (data.m_format == SAMPLEFORMAT_IEEEFP)
                {
                    data.m_pixelFormat = ePixelFormat_R16G16B16A16F;

                    if (data.m_channels == 1 && data.m_photometric != PHOTOMETRIC_MINISBLACK)
                    {
                        data.m_pixelFormat = ePixelFormat_R16F;
                    }
                }
                else
                {
                    data.m_pixelFormat = ePixelFormat_R16G16B16A16;

                    if (data.m_channels == 1 && data.m_photometric != PHOTOMETRIC_MINISBLACK)
                    {
                        data.m_pixelFormat = ePixelFormat_R16;
                    }
                }

                break;
            }

            case 32:
            {
                if (data.m_format == SAMPLEFORMAT_IEEEFP)
                {
                    data.m_pixelFormat = ePixelFormat_R32G32B32A32F;

                    if (data.m_channels == 1 && data.m_photometric != PHOTOMETRIC_MINISBLACK)
                    {
                        data.m_pixelFormat = ePixelFormat_R32F;
                    }
                }

                break;
            }

            default:
                break;
            }

            return data;
        }

        static IImageObject* LoadTIFF(TIFF* tif)
        {
            TiffData data = GetTiffData(tif);
            AZStd::unique_ptr<IImageObject> destImageObject;
            destImageObject.reset(IImageObject::CreateImage(data.m_width, data.m_height,
                1, data.m_pixelFormat));

            uint8* dst;
            uint32 pitch;
            destImageObject->GetImagePointer(0, dst, pitch);

            AZStd::vector<AZ::u8> buf(data.m_bufSize);

            AZ::u8 dstMult = 1;

            // Loop across the image height, one tile at a time
            for (AZ::u32 imageY = 0; imageY < data.m_height; imageY += data.m_tileHeight)
            {
                // If we aren't actually tiled, we'll need to read a scanline here
                if (!data.m_isTiled)
                {
                    if (TIFFReadScanline(tif, buf.data(), imageY) == -1)
                    {
                        AZ_Error("LoadTIFF", false, "Error reading scanline.");
                        return nullptr;
                    }
                }

                // Loop across the image width, one tile at a time
                for (AZ::u32 imageX = 0; imageX < data.m_width; imageX += data.m_tileWidth)
                {
                    // If we *are* tiled, read in a new tile here
                    if (data.m_isTiled)
                    {
                        if (TIFFReadTile(tif, buf.data(), imageX, imageY, 0, 0) == -1)
                        {
                            AZ_Error("LoadTIFF", false, "Error reading tile.");
                            return nullptr;
                        }
                    }

                    // For each pixel in the tile buffer, read it in and convert.
                    for (AZ::u32 tileY = 0; tileY < data.m_tileHeight; ++tileY)
                    {
                        for (AZ::u32 tileX = 0; tileX < data.m_tileWidth; ++tileX)
                        {
                            AZ::u32 srcIdx = ((tileY * data.m_tileWidth) + tileX) * data.m_channels;
                            AZ::u32 destIdx = (((imageY + tileY) * data.m_width) + (imageX + tileX)) * dstMult;

                            switch (data.m_bitsPerPixel)
                            {
                            case 8:
                                Process8BitTiff(dst, buf.data(), destIdx, srcIdx, data, dstMult);
                                break;

                            case 16:
                            {
                                switch (data.m_format)
                                {
                                case SAMPLEFORMAT_INT:
                                case SAMPLEFORMAT_IEEEFP:
                                    Process16BitHDRTiff(reinterpret_cast<AZ::s16*>(dst), reinterpret_cast<AZ::s16*>(buf.data()),
                                        destIdx, srcIdx, data, dstMult);

                                    break;

                                default:
                                    Process16BitTiff(reinterpret_cast<AZ::u16*>(dst), reinterpret_cast<AZ::u16*>(buf.data()),
                                        destIdx, srcIdx, data, dstMult);

                                    break;
                                }

                                break;
                            }

                            case 32:
                                if (data.m_format == SAMPLEFORMAT_IEEEFP)
                                {
                                    Process32BitHDRTiff(reinterpret_cast<float*>(dst), reinterpret_cast<float*>(buf.data()),
                                        destIdx, srcIdx, data, dstMult);
                                }
                                else
                                {
                                    AZ_Error("LoadTIFF", false, "Unknown / unsupported format.");
                                    return nullptr;
                                }

                                break;

                            default:
                                AZ_Error("LoadTIFF", false, "Unknown / unsupported format.");
                                return nullptr;
                            }

                        }
                    }
                }
            }

            return destImageObject.release();
        }

        IImageObject* LoadImageFromTIFF(const AZStd::string& filename)
        {
            TiffFileRead tiffRead(filename);
            TIFF* tif = tiffRead.GetTiff();

            IImageObject* destImageObject = nullptr;

            if (!tif)
            {
                AZ_Warning("Image Processing", false, "%s: Open tiff failed (%s)", __FUNCTION__, filename.c_str());
                return destImageObject;
            }

            uint32 bitsPerChannel = 0;
            uint32 channels = 0;
            uint32 format = 0;
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerChannel);
            TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &format);

            if (channels != 1 && channels != 2 && channels != 3 && channels != 4)
            {
                AZ_Warning("Image Processing", false, "Unsupported TIFF pixel format (channel count: %d)", channels);
                return destImageObject;
            }

            uint32 width = 0;
            uint32 height = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
            if (width <= 0 || height <= 0)
            {
                AZ_Error("Image Processing", false, "%s failed (empty image)", __FUNCTION__);
                return destImageObject;
            }

            destImageObject = LoadTIFF(tif);

            if (destImageObject == nullptr)
            {
                AZ_Error("Image Processing", false, "Failed to read TIFF pixels");
            }

            return destImageObject;
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
                const unsigned int nameSize = (unsigned int)buffer[0];
                ++buffer;

                // get pascal string
                AZStd::string szName(buffer, buffer + nameSize);
                buffer += nameSize;

                // align 2 bytes
                if ((buffer - bufferStart) & 1)
                {
                    ++buffer;
                }

                // get size of resource data
                const unsigned int resDataSize =
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
                    while (iptcPos + 5 < resDataSize)
                    {
                        int marker = iptcBufferStart[iptcPos++];
                        int recordNumber = iptcBufferStart[iptcPos++];
                        int dataSetNumber = iptcBufferStart[iptcPos++];
                        int fieldLength = (iptcBufferStart[iptcPos++] << 8);
                        fieldLength += iptcBufferStart[iptcPos++];

                        // Ignore fields other than SpecialInstructions
                        if (marker != 0x1C || recordNumber != 0x02 || dataSetNumber != 0x28 )
                        {
                            iptcPos += fieldLength;
                            continue;
                        }

                        //save the setting string before close file
                        setting = AZStd::string(iptcBufferStart + iptcPos, iptcBufferStart + iptcPos + fieldLength);
                        return setting;
                    }
                }

                buffer += resDataSize;

                // align 2 bytes
                if ((buffer - bufferStart) & 1)
                {
                    ++buffer;
                }
            }

            return setting;
        }

    }// namespace ImageTIFF

} //namespace ImageProcessing
