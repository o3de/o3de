/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Casting/numeric_cast.h>

#include <ImageLoader/ImageLoaders.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/PixelFormatInfo.h>

#include <QString>

namespace ImageProcessingAtom
{
    namespace TgaLoader
    {
        bool IsExtensionSupported(const char* extension)
        {
            // This is the list of file extensions supported by this loader
            QString ext = QString(extension).toLower();
            return ext == "tga";
        }
       
        // https://www.opennet.ru/docs/formats/targa.pdf
        // http://www.paulbourke.net/dataformats/tga/
        // https://forum.qt.io/topic/74712/qimage-from-tga-with-alpha/11
        // https://forum.qt.io/topic/101971/qimage-and-tga-support-in-c/5

        enum class ImageTypeCode : uint32_t
        {
            // No image data included.
            NoImageData = 0,
            // Uncompressed, color-mapped images.
            ColorMapped = 1, 
            // Uncompressed, RGB images.
            RGB = 2,
            // Uncompressed, black and white images.
            BlackAndWhite = 3,
            // Runlength encoded color-mapped images.
            ColorMappedRLE = 9, 
            // Runlength encoded RGB images.
            RGBRLE = 10,
            // Runlength encoded black and white images.
            BlackAndWhiteRLE = 11
        };

        enum class ImagePixelSize : uint32_t
        {
            Targa8   = 8,
            Targa16 = 16,
            Targa24 = 24,
            Targa32 = 32,
        };

        enum class ImageOrigin : uint32_t
        {
            BottomLeft = 0,
            BottomRight = 1,
            TopLeft = 2,
            TopRight = 3
        };

        const static uint32_t TgaHeaderSize = 18;

        struct TgaHeader
        {
            // Note: the layout and bits size defined in this structure strictly match tga header.
            uint64_t m_idLength : 8;             // Identification size
            uint64_t m_colorMapType : 8;     
            uint64_t m_dataTypeCode : 8;

            // Color Map Specification.
            uint64_t m_colorMapOrigin : 16;
            uint64_t m_colorMapLength : 16;      // total number of color map entries
            uint64_t m_colorMapEntrySize : 8;    // bits per color map entry

            // Image Specification.
            uint64_t m_xOrigin : 16;
            uint64_t m_yOrigin : 16;
            uint64_t m_width : 16;
            uint64_t m_height : 16;
            uint32_t m_bitsPerPixel : 8;         // bits per pixel of the image data
            uint32_t m_imageDescriptor : 8;

            uint32_t GetBitsPerImageData() const
            {
                return aznumeric_cast<uint32_t>(m_bitsPerPixel);
            }

            uint32_t GetBytesPerImageData() const
            {
                return GetBitsPerImageData() / 8;
            }

            uint32_t GetImageBytesSize() const
            {
                uint32_t bytesPerPixel = GetBytesPerImageData();
                uint32_t width = aznumeric_cast<uint32_t>(m_width);
                uint32_t height = aznumeric_cast<uint32_t>(m_height);
                uint32_t imageBytesSize = width * height * bytesPerPixel;
                return imageBytesSize;
            }

            uint32_t GetColorMapBytesSize() const
            {
                uint32_t entryCount = aznumeric_cast<uint32_t>(m_colorMapLength);
                uint32_t entrySize = aznumeric_cast<uint32_t>(m_colorMapEntrySize)/ 8;
                return entryCount * entrySize;
            }

            uint32_t GetBitsPerPixel() const
            {
                if (m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMapped) || m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMappedRLE))
                {
                    return m_colorMapEntrySize;
                }
                return m_bitsPerPixel;
            }

            ImageOrigin GetImageOrigin() const
            {
                return (ImageOrigin)((m_imageDescriptor & 0x30)>>4);
            }
        };

        static_assert(sizeof(TgaHeader) >= TgaHeaderSize, "TgaHeader structure's size should be at least 18 bytes");

        IImageObject* CreateNewImage(const TgaHeader& tgaHeader, const AZStd::vector<uint8>& imageData, const AZStd::vector<uint8>& colorMapData);

        // Read image data
        bool ReadImageData(const TgaHeader& tgaHeader, AZ::IO::SystemFileStream& imageFileStream, AZStd::vector<uint8>& imageData)
        {
            uint32_t bytesPerImagePixel = tgaHeader.GetBytesPerImageData();
            uint32_t imageBytesSize = tgaHeader.GetImageBytesSize();
            bool isRLE = tgaHeader.m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::RGBRLE) || tgaHeader.m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMappedRLE);

            imageData.resize_no_construct(imageBytesSize);

            if (!isRLE)
            {
                if (imageFileStream.Read(imageBytesSize, imageData.data()) != imageBytesSize)
                {
                    return false;
                }
            }
            else
            {
                uint8 chunkHeader;
                uint8 byteData[5];
                uint32_t imageDataOffset = 0;

                do
                {
                    if (imageFileStream.Read(sizeof(chunkHeader), reinterpret_cast<char*>(&chunkHeader)) != sizeof(chunkHeader))
                    {
                        return false;
                    }

                    if (chunkHeader >> 7) // repeat count
                    {
                        // the lower 7 bits is the repeat count - 1
                        uint8 repeatCount = (chunkHeader&0x7f) + 1;

                        // The followed data is repeating repeatCount times
                        if (imageFileStream.Read(bytesPerImagePixel, reinterpret_cast<char*>(byteData)) != bytesPerImagePixel)
                        {
                            return false;
                        }

                        for (uint8 count = 0; count < repeatCount; count++)
                        {
                            memcpy(imageData.data() + imageDataOffset, byteData, bytesPerImagePixel);
                            imageDataOffset += bytesPerImagePixel;
                        }
                    }
                    else // data count
                    {
                        uint8 dataCount = chunkHeader + 1;

                        // The followed number of data need to be read.
                        uint32_t readDataBytes = bytesPerImagePixel * dataCount;
                        if (imageFileStream.Read(readDataBytes, imageData.data() + imageDataOffset) != readDataBytes)
                        {
                            return false;
                        }
                        imageDataOffset += readDataBytes;
                    }
                } while (imageDataOffset < imageBytesSize);
            }

            return true;
        }

        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
            // open the file
            AZ::IO::SystemFileStream imageFileStream(filename.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!imageFileStream.IsOpen())
            {
                AZ_Warning("Image Processing", false, "TgaLoader: failed to open file %s", filename.c_str());
                return nullptr;
            }

            // read in the header
            TgaHeader tgaHeader;
            
            if (imageFileStream.Read(TgaHeaderSize, &tgaHeader) != TgaHeaderSize)
            {
                AZ_Warning("Image Processing", false, "TgaLoader: failed to read file header %s", filename.c_str());
                return nullptr;
            }

            // only support rgb or colormapped formats
            if (tgaHeader.m_dataTypeCode != static_cast<uint64_t>(ImageTypeCode::ColorMapped)
                && tgaHeader.m_dataTypeCode != static_cast<uint64_t>(ImageTypeCode::RGB)
                && tgaHeader.m_dataTypeCode != static_cast<uint64_t>(ImageTypeCode::ColorMappedRLE)
                && tgaHeader.m_dataTypeCode != static_cast<uint64_t>(ImageTypeCode::RGBRLE))
            {
                AZ_Warning("Image Processing", false, "TgaLoader: unsupported type code [%u] of TGA file %s. Only support RGB(RLE) or color mapped (RLE) tga images",
                    tgaHeader.m_dataTypeCode, filename.c_str());
                return nullptr;
            }

            // only support 24bits or 32 bits pixel format
            uint32_t pixelBits = tgaHeader.GetBitsPerPixel();
            if (pixelBits != static_cast<uint32_t>(ImagePixelSize::Targa24) && pixelBits != static_cast<uint32_t>(ImagePixelSize::Targa32))
            {
                AZ_Warning("Image Processing", false, "TgaLoader: unsupported pixel size [%u] of TGA file %s. Only support 24bits or 32bits color",
                    pixelBits, filename.c_str());
                return nullptr;
            }

            // validate image data pixel size for color mapped
            if ( (tgaHeader.m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMapped) || tgaHeader.m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMappedRLE))
                 && tgaHeader.GetBytesPerImageData() > 2)
            {
                AZ_Warning("Image Processing", false, "TgaLoader: invalid image pixel size [%u] for color mapped image of TGA file %s. It should be 1 or 2",
                    tgaHeader.GetBytesPerImageData(), filename.c_str());
                return nullptr;
            }

            // Skip image identification data if there are any
            imageFileStream.Seek(tgaHeader.m_idLength,  AZ::IO::GenericStream::SeekMode::ST_SEEK_CUR);

            // Read color map if there is one
            AZStd::vector<uint8> colorMap;
            if (tgaHeader.m_colorMapType != 0) // color map exists
            {
                uint32_t colorMapSize = tgaHeader.GetColorMapBytesSize();
                colorMap.resize_no_construct(colorMapSize);
                imageFileStream.Read(colorMapSize, colorMap.data());
            }

            // Read image data
            AZStd::vector<uint8> imageData;
            bool success = ReadImageData(tgaHeader, imageFileStream, imageData);
            if (!success)
            {
                AZ_Warning("Image Processing", false, "TgaLoader: failed to read read image data from file %s", filename.c_str());
                return nullptr;
            }
            
            return CreateNewImage(tgaHeader, imageData, colorMap); 
        }

        IImageObject* CreateNewImage(const TgaHeader& tgaHeader, const AZStd::vector<uint8>& imageData, const AZStd::vector<uint8>& colorMapData)
        {
            uint32_t bytesPerPixel = tgaHeader.GetBitsPerPixel()/8;

            EPixelFormat pixelFormat = ePixelFormat_R8G8B8A8;
            if (bytesPerPixel == 3)
            {
                pixelFormat = ePixelFormat_R8G8B8;
            }

            IImageObject* pImage = IImageObject::CreateImage(tgaHeader.m_width, tgaHeader.m_height, 1, pixelFormat);

            // copy data from pixelContent to the ImageObject
            uint8* pDst;
            uint32 dwPitch;
            pImage->GetImagePointer(0, pDst, dwPitch);

            const uint32 pixelCount = pImage->GetPixelCount(0);
            bool useColorMap = tgaHeader.m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMapped) || tgaHeader.m_dataTypeCode == static_cast<uint64_t>(ImageTypeCode::ColorMappedRLE);
            ImageOrigin imageOrigin = tgaHeader.GetImageOrigin();

            // lambda function to find the index of image data based the origin
            auto GetImagePixelIndex = [tgaHeader] (ImageOrigin imageOrigin, uint32 dstIndex)
            {
                uint32 reIndex = dstIndex;
                uint32 imageWidth = aznumeric_cast<uint32>(tgaHeader.m_width);
                uint32 imageHeight = aznumeric_cast<uint32>(tgaHeader.m_height);
                switch (imageOrigin)
                {
                case ImageOrigin::BottomLeft:
                    {
                        auto x = dstIndex % imageWidth;
                        auto y = imageHeight - 1 - dstIndex/imageWidth;
                        reIndex = y * imageWidth + x;
                        break;
                    }
                case ImageOrigin::BottomRight:
                    {
                        auto x = imageWidth - 1 - dstIndex % imageWidth;
                        auto y = imageHeight - 1 - dstIndex/imageWidth;
                        reIndex = y * imageWidth + x;
                        break;
                    }
                case ImageOrigin::TopRight:
                    {
                        auto x = imageWidth - 1 - dstIndex % imageWidth;
                        auto y = dstIndex/imageWidth;
                        reIndex = y * imageWidth + x;
                        break;
                    }

                }
                return reIndex;
            };

            for (uint32 i = 0; i < pixelCount; ++i)
            {
                uint32 srcIndex = GetImagePixelIndex(imageOrigin, i);
                if (useColorMap)
                {
                    uint32_t colorMapIndex;
                    if (tgaHeader.GetBytesPerImageData() == 1)
                    {
                        colorMapIndex = imageData[srcIndex];
                    }
                    else 
                    {   // 2 bytes
                        colorMapIndex = ((uint16*)imageData.data())[srcIndex];
                    }

                    memcpy(pDst + i*bytesPerPixel, colorMapData.data() + colorMapIndex*bytesPerPixel, bytesPerPixel);
                }
                else
                {
                    memcpy(pDst + i*bytesPerPixel, imageData.data() + srcIndex*bytesPerPixel, bytesPerPixel);
                }

                // swap R and B
                AZStd::swap(pDst[i*bytesPerPixel], pDst[i*bytesPerPixel + 2]);
            }

            return pImage;
        }

    }// namespace TgaLoader
} //namespace ImageProcessingAtom

