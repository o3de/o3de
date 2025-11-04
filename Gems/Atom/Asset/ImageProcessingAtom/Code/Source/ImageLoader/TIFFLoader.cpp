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
#include <Processing/PixelFormatInfo.h>

#include <QString>

#include <tiffio.h>  // TIFF library

namespace ImageProcessingAtom
{
    namespace TIFFLoader
    {
#ifdef AZ_ENABLE_TRACING
        static constexpr int TiffMaxMessageSize = 1024;
        // Note: the fatal errors are processed in LoadImageFromTIFF function.
        // We only report the errors as warning here. 
        static void ImageProcessingTiffErrorHandler(const char* module, const char* format, va_list argList)
        {
            char buffer[TiffMaxMessageSize];
            azvsnprintf(buffer, TiffMaxMessageSize, format, argList);
            AZ_Warning(module, false, buffer);
        }
#endif

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

        // loads image from a TIFF structure and converts to an Atom image.
        static IImageObject* LoadImageFromTIFFInternal(TIFF* tif);

        // Based on the input TIFF format, choose an appropriate output pixel format.
        static EPixelFormat GetOutputPixelFormat(uint32_t dwChannels, uint32_t dwBitsPerChannel, uint32_t dwFormat);

        IImageObject* LoadImageFromTIFF(const AZStd::string& filename)
        {
#ifdef AZ_ENABLE_TRACING
            // Reroute the TIFF loader Error Handler so that any load errors are recorded.
            // There is also a warning handler that can get rerouted via TIFFSetWarningHandler, but the warnings include noisy notices
            // like 'tiff tag X unsupported', so it isn't currently hooked up here.
            TIFFSetErrorHandler(ImageProcessingTiffErrorHandler);
#endif

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
            TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &dwChannels);
            TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &dwBitsPerChannel);
            TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &dwFormat);

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

            bool validFormat = false;

            // Currently, we only support uint/int TIFFs with 8 or 16 bits per channel and float TIFFs with 16 or 32 bits per channel.
            switch (dwFormat)
            {
                case SAMPLEFORMAT_UINT:
                case SAMPLEFORMAT_INT:
                    if ((dwBitsPerChannel == 8) || (dwBitsPerChannel == 16))
                    {
                        pRet = LoadImageFromTIFFInternal(tif);
                        validFormat = true;
                    }
                    break;
                case SAMPLEFORMAT_IEEEFP:
                    if ((dwBitsPerChannel == 16) || (dwBitsPerChannel == 32))
                    {
                        pRet = LoadImageFromTIFFInternal(tif);
                        validFormat = true;
                    }
                    break;
                default:
                    // Unsupported format, invalid.
                    break;
            }

            if (!validFormat)
            {
                AZ_Error(
                    "Image Processing", false, "File %s has unsupported TIFF pixel format. sample channels: %d,\
                            bits per channel: %d, sample format: %d",
                    filename.c_str(), dwChannels, dwBitsPerChannel, dwFormat);
                return pRet;
            }

            if (pRet == nullptr)
            {
                AZ_Error("Image Processing", false, "Failed to read TIFF pixels");
                return pRet;
            }

            return pRet;
        }

        static EPixelFormat GetOutputPixelFormat(uint32_t numChannels, uint32_t bitsPerChannel, uint32_t channelFormat)
        {
            // The output formats we want to convert the TIFF into, based on the number of input channels, bit depth, and format.
            // The 2-channel choices below are arbitrary, we might someday want to consider converting them to 2-channel outputs.

            static constexpr EPixelFormat output8BitIntFormats[4] = {
                ePixelFormat_R8, // 1 channel in goes to 1 channel out
                ePixelFormat_R8G8B8X8, // 2 channels in becomes RGBA with A=100%
                ePixelFormat_R8G8B8X8, // 3 channels in becomes RGBA with A=100%
                ePixelFormat_R8G8B8A8 // 4 channels in goes to 4 channels out
            };

            static constexpr EPixelFormat output16BitIntFormats[4] = {
                ePixelFormat_R16, // 1 channel in goes to 1 channel out
                ePixelFormat_R16G16B16A16, // 2 channels in becomes RGBA with A=100%
                ePixelFormat_R16G16B16A16, // 3 channels in becomes RGBA with A=100%
                ePixelFormat_R16G16B16A16 // 4 channels in goes to 4 channels out
            };

            static constexpr EPixelFormat output16BitFloatFormats[4] = {
                ePixelFormat_R16F, // 1 channel in goes to 1 channel out
                ePixelFormat_R16G16B16A16F, // 2 channels in becomes RGBA with A=100%
                ePixelFormat_R16G16B16A16F, // 3 channels in becomes RGBA with A=100%
                ePixelFormat_R16G16B16A16F // 4 channels in goes to 4 channels out
            };

            static constexpr EPixelFormat output32BitFloatFormats[4] = {
                ePixelFormat_R32F, // 1 channel in goes to 1 channel out
                ePixelFormat_R32G32B32A32F, // 2 channels in becomes RGBA with A=100%
                ePixelFormat_R32G32B32A32F, // 3 channels in becomes RGBA with A=100%
                ePixelFormat_R32G32B32A32F // 4 channels in goes to 4 channels out
            };

            bool isIntFormat = (channelFormat == SAMPLEFORMAT_INT) || (channelFormat == SAMPLEFORMAT_UINT);

            if ((bitsPerChannel == 8) && isIntFormat)
            {
                return output8BitIntFormats[numChannels - 1];
            }
            else if ((bitsPerChannel == 16) && isIntFormat)
            {
                return output16BitIntFormats[numChannels - 1];
            }
            else if ((bitsPerChannel == 16) && (channelFormat == SAMPLEFORMAT_IEEEFP))
            {
                return output16BitFloatFormats[numChannels - 1];
            }
            else if ((bitsPerChannel == 32) && (channelFormat == SAMPLEFORMAT_IEEEFP))
            {
                return output32BitFloatFormats[numChannels - 1];
            }
            else
            {
                // Error, unsupported format.
                return ePixelFormat_Unknown;
            }
        }

        static IImageObject* LoadImageFromTIFFInternal(TIFF* tif)
        {
            uint32_t bitsPerChannel = 0;
            uint32_t numChannels = 0;
            uint32_t photometricFormat = 0;
            uint32_t sampleFormat = 0;
            TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerChannel);
            TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &numChannels);
            TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &photometricFormat);
            TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

            uint32_t inputImageWidth = 0;
            uint32_t inputImageHeight = 0;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &inputImageWidth);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &inputImageHeight);

            uint32_t tileWidth = 0;
            uint32_t tileHeight = 0;
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileHeight);

            bool isTiled = (tileWidth > 0) && (tileHeight > 0);
            bool isIntFormat = (sampleFormat == SAMPLEFORMAT_INT) || (sampleFormat == SAMPLEFORMAT_UINT);

            EPixelFormat outputPixelFormat = GetOutputPixelFormat(numChannels, bitsPerChannel, sampleFormat);
            if (outputPixelFormat == ePixelFormat_Unknown)
            {
                return nullptr;
            }

            if ((photometricFormat == PHOTOMETRIC_SEPARATED) && (sampleFormat == SAMPLEFORMAT_IEEEFP))
            {
                AZ_Error("Image Processing", false, "Separated Photometric format isn't supported with floating-point images.");
                return nullptr;
            }

            uint32_t dstChannels = CPixelFormats::GetInstance().GetPixelFormatInfo(outputPixelFormat)->nChannels;
            IImageObject* pRet = IImageObject::CreateImage(inputImageWidth, inputImageHeight, 1, outputPixelFormat);

            uint8_t* dst;
            uint32_t dwPitch;
            pRet->GetImagePointer(0, dst, dwPitch);

            // Determine if this is a scanline-based or tile-based TIFF, and size our temporary buffer appropriately.
            size_t bufSize = 0;
            if (isTiled)
            {
                // Tiled TIFF, so our buffer needs to be tile-sized
                bufSize = TIFFTileSize(tif);
            }
            else
            {
                // Scanline TIFF, so our buffer needs to be scanline-sized.
                bufSize = TIFFScanlineSize(tif);

                // For our processing loops, we'll treat scanlines like a tile of 1 x width size.
                tileHeight = 1;
                tileWidth = inputImageWidth;
            }

            AZStd::vector<uint8_t> buf(bufSize);

            // There are two types of 32-bit floating point TIF semantics.  Paint programs tend to use values in the 0.0 - 1.0 range.
            // GeoTIFF files use values where 1.0 = 1 meter by default, but also have an optional ZScale parameter to provide additional
            // scaling control.
            // By default, we'll assume this is a regular TIFF that we want to leave in the 0.0 - 1.0 range.
            float pixelValueScale = 1.0f;

            // Check to see if it's a GeoTIFF, and if so, whether or not it has the ZScale parameter.
            // Defined in GeoTIFF format -
            // http://web.archive.org/web/20160403164508/http://www.remotesensing.org/geotiff/spec/geotiffhome.html
            // Used to get the X, Y, Z scales from a GeoTIFF file
            bool isGeoTIFF = false;
            {
                uint32 tagCount = 0;
                double* pixelScales = NULL;
                static constexpr int GEOTIFF_MODELPIXELSCALE_TAG = 33550;
                if (TIFFGetField(tif, GEOTIFF_MODELPIXELSCALE_TAG, &tagCount, &pixelScales) == 1)
                {
                    isGeoTIFF = true;

                    // if there's an xyz scale, and the Z scale isn't 0, let's use it.
                    if ((tagCount == 3) && (pixelScales != NULL) && (pixelScales[2] != 0.0f))
                    {
                        pixelValueScale = static_cast<float>(pixelScales[2]);
                    }
                }
            }

            // Track min/max values for GeoTIFFs so that we can scale the values into the 0-1 range.
            float minChannelValue = AZStd::numeric_limits<float>::max();
            float maxChannelValue = AZStd::numeric_limits<float>::lowest();

            // Copy one channel of one pixel from source to destination, and optionally invert the value.
            auto CopyPixelChannel = [bitsPerChannel, pixelValueScale, &buf, &dst, &minChannelValue, &maxChannelValue]
                (uint32_t destPixelChannelIndex, uint32_t srcPixelChannelIndex, bool invert = false)
            {
                if (bitsPerChannel == 8)
                {
                    dst[destPixelChannelIndex] = invert ? (0xFF - buf[srcPixelChannelIndex]) : buf[srcPixelChannelIndex];
                }
                else if (bitsPerChannel == 16)
                {
                    // Alias the base pointers to a 16-bit channel type, then use the channel index to get to the correct pixel & channel
                    uint16_t* buf16 = reinterpret_cast<uint16_t*>(buf.data());
                    uint16_t* dst16 = reinterpret_cast<uint16_t*>(dst);
                    dst16[destPixelChannelIndex] = invert ? (0xFFFF - buf16[srcPixelChannelIndex]) : buf16[srcPixelChannelIndex];
                }
                else
                {
                    // Alias the base pointers to a 32-bit channel type, then use the channel index to get to the correct pixel & channel
                    float* buf32 = reinterpret_cast<float*>(buf.data());
                    float* dst32 = reinterpret_cast<float*>(dst);

                    // GeoTIFFs might have a pixel scale, so apply it.
                    const float scaledValue = buf32[srcPixelChannelIndex] * pixelValueScale;

                    // Track min/max values, but exclude the lowest float value, as that might be a "no data" value for GeoTIFFs.
                    if (scaledValue > AZStd::numeric_limits<float>::lowest())
                    {
                        minChannelValue = AZStd::min(minChannelValue, scaledValue);
                        maxChannelValue = AZStd::max(maxChannelValue, scaledValue);
                    }

                    // We ignore the inversion flag for floats, it will always be false.
                    dst32[destPixelChannelIndex] = scaledValue;
                }
            };

            // Set one channel of one pixel in the destination to a specific value.
            auto SetPixelChannel = [bitsPerChannel, &dst](uint32_t dstIdx, uint32_t value)
            {
                if (bitsPerChannel == 8)
                {
                    dst[dstIdx] = static_cast<uint8_t>(value);
                }
                else if (bitsPerChannel == 16)
                {
                    // Alias the base pointers to a 16-bit channel type, then use the channel index to get to the correct pixel & channel
                    uint16_t* dst16 = reinterpret_cast<uint16_t*>(dst);
                    dst16[dstIdx] = static_cast<uint16_t>(value);
                }
                else
                {
                    // Alias the base pointers to a 32-bit channel type, then use the channel index to get to the correct pixel & channel
                    float* dst32 = reinterpret_cast<float*>(dst);
                    dst32[dstIdx] = static_cast<float>(value);
                }
            };

            // Loop across the image height, one tile at a time
            for (uint32_t imageY = 0; imageY < inputImageHeight; imageY += tileHeight)
            {
                // Loop across the image width, one tile at a time
                for (uint32 imageX = 0; imageX < inputImageWidth; imageX += tileWidth)
                {
                    // Either read in a tile or a scanline
                    [[maybe_unused]] auto result = isTiled?
                        TIFFReadTile(tif, buf.data(), imageX, imageY, 0, 0):
                        TIFFReadScanline(tif, buf.data(), imageY);

                    // non-fatal error, only print the warning
                    // For details: https://github.com/o3de/o3de/pull/8929
                    AZ_Warning("TIFFLoader", !(result == -1), "Read tiff image data from %s error at row %d", TIFFFileName(tif), imageY);

                    // Convert each pixel in the scanline / tile buffer.
                    // The image might not be evenly divisible by tile height/width, so don't process any pixels outside those bounds.
                    for (uint32 tileY = 0; (tileY < tileHeight) && ((imageY + tileY) < inputImageHeight); tileY++)
                    {
                        for (uint32 tileX = 0; (tileX < tileWidth) && ((imageX + tileX) < inputImageWidth); tileX++)
                        {
                            // Calculate the buffer start index for the source and destination pixels.
                            // These indices are by channel, not by byte, so for example a 2x2 R16G16B16A16 image will have
                            // pixel channel indices of 0, 4, 8, 12. If they were by byte, they'd be 0, 8, 16, 24.
                            // Also, note that the destination image provides a "pitch" value that's the number of bytes per row,
                            // which could include padding. Since that value is in bytes, we divide by bytes per channel so that
                            // our index is back in channel range, not byte range.
                            uint32 srcPixelChannelIndex = ((tileY * tileWidth) + tileX) * numChannels;
                            uint32 destPixelChannelIndex =
                                ((imageY + tileY) * (dwPitch / (bitsPerChannel / 8))) +
                                ((imageX + tileX) * dstChannels);

                            if (numChannels == 1)
                            {
                                // One channel, perform a straight copy.
                                CopyPixelChannel(destPixelChannelIndex, srcPixelChannelIndex);
                            }
                            else if (numChannels == 2)
                            {
                                if (photometricFormat == PHOTOMETRIC_SEPARATED)
                                {
                                    // convert CMY to RGB (PHOTOMETRIC_SEPARATED refers to inks in TIFF, the value is inverted)
                                    constexpr bool invert = true;
                                    CopyPixelChannel(destPixelChannelIndex + 0, srcPixelChannelIndex + 0, invert);
                                    CopyPixelChannel(destPixelChannelIndex + 1, srcPixelChannelIndex + 1, invert);
                                    SetPixelChannel(destPixelChannelIndex + 2, 0x00000000);
                                    SetPixelChannel(destPixelChannelIndex + 3, isIntFormat ? 0xFFFFFFFF : 1);
                                }
                                else
                                {
                                    // Not separated, so just copy the two channels, and fill in the other two channels with defaults.
                                    CopyPixelChannel(destPixelChannelIndex + 0, srcPixelChannelIndex + 0);
                                    CopyPixelChannel(destPixelChannelIndex + 1, srcPixelChannelIndex + 1);
                                    SetPixelChannel(destPixelChannelIndex + 2, 0x00000000);
                                    SetPixelChannel(destPixelChannelIndex + 3, isIntFormat ? 0xFFFFFFFF : 1);
                                }
                            }
                            else if (numChannels == 3)
                            {
                                // 3 channels, copy over RGB and fill in Alpha with a default.
                                CopyPixelChannel(destPixelChannelIndex + 0, srcPixelChannelIndex + 0);
                                CopyPixelChannel(destPixelChannelIndex + 1, srcPixelChannelIndex + 1);
                                CopyPixelChannel(destPixelChannelIndex + 2, srcPixelChannelIndex + 2);
                                SetPixelChannel(destPixelChannelIndex + 3, isIntFormat ? 0xFFFFFFFF : 1);
                            }
                            else
                            {
                                // 4 channels, just perform a straight copy.
                                CopyPixelChannel(destPixelChannelIndex + 0, srcPixelChannelIndex + 0);
                                CopyPixelChannel(destPixelChannelIndex + 1, srcPixelChannelIndex + 1);
                                CopyPixelChannel(destPixelChannelIndex + 2, srcPixelChannelIndex + 2);
                                CopyPixelChannel(destPixelChannelIndex + 3, srcPixelChannelIndex + 3);
                            }
                        }
                    }
                }
            }

            // A GeoTIFF image contains real-world height values, so the values could potentially range from roughly +/- 10000 meters.
            // To make this data usable in-engine, it will get scaled to 0.0 - 1.0, based on the min/max values found in the file.
            if (isGeoTIFF && (sampleFormat == SAMPLEFORMAT_IEEEFP))
            {
                float* dst32 = reinterpret_cast<float*>(dst);

                for (uint32 imageY = 0; imageY < inputImageHeight; imageY++)
                {
                    for (uint32 imageX = 0; imageX < inputImageWidth; imageX++)
                    {
                        uint32 pixelChannelIndex = (imageY * (dwPitch / (bitsPerChannel / 8))) + (imageX * dstChannels);
                        dst32[pixelChannelIndex] =
                            AZStd::clamp((dst32[pixelChannelIndex] - minChannelValue) / (maxChannelValue - minChannelValue), 0.0f, 1.0f);
                    }
                }
            }

            return pRet;
        }
    }// namespace ImageTIFF
} //namespace ImageProcessingAtom

