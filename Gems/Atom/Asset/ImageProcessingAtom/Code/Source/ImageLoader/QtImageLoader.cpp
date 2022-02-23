/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageLoader/ImageLoaders.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <ImageBuilderBaseType.h>

//  warning C4251: class QT_Type needs to have dll-interface to be used by clients of class 'QT_Type'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QImage>
#include <QImageReader>
#include <QFileInfo>
AZ_POP_DISABLE_WARNING

///////////////////////////////////////////////////////////////////////////////////

namespace ImageProcessingAtom
{
    namespace QtImageLoader
    {
        /*
        * https://www.opennet.ru/docs/formats/targa.pdf
        * http://www.paulbourke.net/dataformats/tga/
        */
        enum class ImageTypeCode : AZ::u8
        {
            // No image data included.
            NoImageData = 0, // not supported?
            // Uncompressed, color-mapped images.
            ColorMappedUncompressed = 1, // not supported?
            // Uncompressed, RGB images.
            UnmappedRGB = 2,
            // Uncompressed, black and white images.
            BlackAndWhiteUncompressed = 3, // not supported?
            // Runlength encoded color-mapped images.
            ColorMappedRLE = 9, // not supported?
            // Runlength encoded RGB images.
            RGBRLE = 10,
            // Compressed, black and white images.
            BlackAndWhiteCompressed = 11, // not supported?
            // Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
            ColorMappedCompressedRLE = 32, // not supported?
            // Compressed color-mapped data, using Huffman, Delta, and runlength encoding.4 - pass quadtree - type process.
            ColorMappedCompressedRLEWith4Pass = 33, // not supported?
        };
        /*
         * https://www.opennet.ru/docs/formats/targa.pdf
         * http://www.paulbourke.net/dataformats/tga/
         */
        enum class ImagePixelSize : AZ::u8
        {
            Targa8   = 8,
            Targa16 = 16,
            Targa24 = 24,
            Targa32 = 32,
        };
#pragma pack(1)
        /*
         * https://www.opennet.ru/docs/formats/targa.pdf
         * http://www.paulbourke.net/dataformats/tga/
         */
        struct TgaHeader
        {
            AZ::u64 m_idLength : 8;
            AZ::u64 m_colorMapType : 8;
            AZ::u64 m_dataTypeCode : 8;
            AZ::u64 m_colorMapOrigin : 16;
            AZ::u64 m_colorMapLength : 16;
            AZ::u64 m_colorMapDepth : 8;
            AZ::u64 m_xOrigin : 16;
            AZ::u64 m_yOrigin : 16;
            AZ::u64 m_width : 16;
            AZ::u64 m_height : 16;
            AZ::u32 m_bitsPerPixel : 8;
            AZ::u32 m_imageDescriptor : 8;

            ImageTypeCode GetDataTypeCode()
            {
                ImageTypeCode imageTypeCode = static_cast<ImageTypeCode>(m_dataTypeCode);
                return imageTypeCode;
            }

            bool IsDataTypeCodeSupported()
            {
                ImageTypeCode imageTypeCode = GetDataTypeCode();
                return imageTypeCode == ImageTypeCode::UnmappedRGB || imageTypeCode == ImageTypeCode::RGBRLE;
            }

            ImagePixelSize GetImagePixelSize() const
            {
                ImagePixelSize pixelSize = static_cast<ImagePixelSize>(m_bitsPerPixel);
                return pixelSize;
            }

            bool IsImagePixelSizeSupported()
            {
                ImagePixelSize pixelSize = GetImagePixelSize();
                return pixelSize == ImagePixelSize::Targa24 || pixelSize == ImagePixelSize::Targa32;
            }

            AZ::u8 GetBitsPerPixel() const
            {
                return aznumeric_cast<AZ::u8>(m_bitsPerPixel);
            }

            AZ::u32 GetBytesPerPixel() const
            {
                return GetBitsPerPixel() / 8;
            }

            AZ::u32 GetImageBytesSize()
            {
                AZ::u32 bytesPerPixel = GetBytesPerPixel();
                AZ::u32 width = aznumeric_cast<AZ::u32>(m_width);
                AZ::u32 height = aznumeric_cast<AZ::u32>(m_height);
                AZ::u32 imageBytesSize = width * height * bytesPerPixel;
                return imageBytesSize;
            }
        };
#pragma pack()

        static void FillImageContent(
            const TgaHeader& tgaHeader, const AZ::u8* byteData, AZStd::vector<AZ::u8>& pixelContent, AZ::u32& byteIndex);
        static void CreateNewImage(const TgaHeader& tgaHeader, const AZStd::vector<AZ::u8>& pixelContent, QImage& image);
        static bool LoadTgaImage(const AZStd::string& filename, QImage& image);

        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
            //try to open the image
            QImage qimage(filename.c_str());
            if (qimage.isNull())
            {
                QString suffix = QFileInfo(filename.c_str()).suffix().toLower();
                if (suffix == "tga")
                {
                    if (!LoadTgaImage(filename, qimage))
                    {
                        AZ_Error("ImageProcessing", false, "Failed to load [%s] via TGA Image", filename.c_str());
                        return nullptr;
                    }
                }
                else
                {
                    AZ_Error("ImageProcessing", false, "Failed to load [%s] via QImage", filename.c_str());
                    return nullptr;
                }
                
            }

            //convert to format which compatiable our pixel format
            if (qimage.format() != QImage::Format_RGBA8888)
            {
                qimage = qimage.convertToFormat(QImage::Format_RGBA8888);
            }

            //create a new image object
            IImageObject* pImage = IImageObject::CreateImage(qimage.width(), qimage.height(), 1,
                ePixelFormat_R8G8B8A8);

            //get a pointer to the image objects pixel data
            uint8* pDst;
            uint32 dwPitch;
            pImage->GetImagePointer(0, pDst, dwPitch);

            //copy the qImage into the image object
            for (uint32 dwY = 0; dwY < (uint32)qimage.height(); ++dwY)
            {
                uint8* dstLine = &pDst[dwPitch * dwY];
                uchar* srcLine = qimage.scanLine(dwY);
                memcpy(dstLine, srcLine, dwPitch);
            }
            return pImage;
        }

        // https://forum.qt.io/topic/74712/qimage-from-tga-with-alpha/11
        // https://forum.qt.io/topic/101971/qimage-and-tga-support-in-c/5
        // http://www.paulbourke.net/dataformats/tga/
        bool LoadTgaImage(const AZStd::string& filename, QImage& image)
        {
            if (image.load(filename.c_str()))
            {
                return true;
            }

            // open the file
            AZ::IO::SystemFile imageFile;
            imageFile.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);

            if (!imageFile.IsOpen())
            {
                AZ_Warning("Image Processing", false, "Failed to open file %s", filename.c_str());
                return false;
            }

            AZ::IO::SystemFileStream imageFileStream(&imageFile, true);
            if (!imageFileStream.IsOpen())
            {
                AZ_Warning("Image Processing", false, "Failed to create file stream %s", filename.c_str());
                return false;
            }

            // read in the header
            TgaHeader tgaHeader;
            
            if (imageFileStream.Read(sizeof(tgaHeader), &tgaHeader) != sizeof(tgaHeader))
            {
                AZ_Warning("Image Processing", false, "Failed to read file header %s", filename.c_str());
                return false;
            }

            if (tgaHeader.m_dataTypeCode != ImageTypeCode::UnmappedRGB && tgaHeader.m_dataTypeCode != ImageTypeCode::RGBRLE)
            {
                AZ_Warning("Image Processing", false, "Unsupported type code [%d] of TGA file %s", tgaHeader.m_dataTypeCode, filename.c_str());
                return false;
            }

            if (tgaHeader.m_bitsPerPixel != ImagePixelSize::Targa24 && tgaHeader.m_bitsPerPixel != ImagePixelSize::Targa32)
            {
                AZ_Warning(
                    "Image Processing", false, "Unsupported bits per pixel [%d], type code [%d] of TGA file %s", tgaHeader.m_bitsPerPixel,
                    tgaHeader.m_dataTypeCode, filename.c_str());
                return false;
            }

            AZ::u32 bytesPerPixel = (AZ::u8)tgaHeader.m_bitsPerPixel / 8;
            AZ::u32 imageBytesSize = tgaHeader.m_width * tgaHeader.m_height * bytesPerPixel;

            AZStd::vector<AZ::u8> pixelContent;
            pixelContent.resize(imageBytesSize);

            // jump to the data block
            AZ::IO::OffsetType startOffsetOfDataBlocks = tgaHeader.m_idLength + tgaHeader.m_colorMapType * tgaHeader.m_colorMapLength;
            imageFileStream.Seek(startOffsetOfDataBlocks, AZ::IO::SystemFileStream::SeekMode::ST_SEEK_CUR);

            if (tgaHeader.m_dataTypeCode == ImageTypeCode::UnmappedRGB)
            {
                if (imageFileStream.Read(imageBytesSize, reinterpret_cast<char*>(pixelContent.data())) != imageBytesSize)
                {
                    AZ_Warning("Image Processing", false, "failed to read file content %s", filename.c_str());
                    return false;
                }
                
                CreateNewImage(tgaHeader, pixelContent, image);
                return true;
            }
            
            //  if compressed 24 or 32 bit
            if (tgaHeader.m_dataTypeCode == ImageTypeCode::RGBRLE) // compressed
            {
                AZ::u8 chunkHeader;
                AZ::u8 byteData[5];
                AZ::u32 byteIndex = 0;

                do
                {
                    if (imageFileStream.Read(sizeof(chunkHeader), reinterpret_cast<char*>(&chunkHeader)) != sizeof(chunkHeader))
                    {
                        AZ_Warning("Image Processing", false, "failed to read trunk header %s", filename.c_str());
                        return false;
                    }

                    if (chunkHeader >> 7) // repeat count
                    {
                        // just use the first 7 bits
                        chunkHeader = AZ::u8(chunkHeader << 1) >> 1;

                        if (imageFileStream.Read(bytesPerPixel, reinterpret_cast<char*>(byteData)) != bytesPerPixel)
                        {
                            AZ_Warning("Image Processing", false, "failed to read trunk content %s", filename.c_str());
                            return false;
                        }

                        FillImageContent(tgaHeader, byteData, pixelContent, byteIndex);
                    }
                    else // data count
                    {
                        // just use the first 7 bits
                        chunkHeader = AZ::u8(chunkHeader << 1) >> 1;

                        for (AZ::u8 i = 0; i <= chunkHeader; i++)
                        {
                            if (imageFileStream.Read(bytesPerPixel, reinterpret_cast<char*>(byteData)) != bytesPerPixel)
                            {
                                AZ_Warning("Image Processing", false, "failed to read trunk content %s", filename.c_str());
                                return false;
                            }

                            FillImageContent(tgaHeader, byteData, pixelContent, byteIndex);
                        }
                    }
                } while (byteIndex < imageBytesSize);
                
                CreateNewImage(tgaHeader, pixelContent, image);
            }
            
            // not useable format
            return false;
        }
        
        void FillImageContent(
            const TgaHeader& tgaHeader, const AZ::u8* byteData, AZStd::vector<AZ::u8>& pixelContent, AZ::u32& byteIndex)
        {
            pixelContent.at(byteIndex++) = byteData[0];
            pixelContent.at(byteIndex++) = byteData[1];
            pixelContent.at(byteIndex++) = byteData[2];
            if (tgaHeader.m_bitsPerPixel == ImagePixelSize::Targa32)
            {
                pixelContent.at(byteIndex++) = byteData[3];
            }
        }
        
        void CreateNewImage(const TgaHeader& tgaHeader, const AZStd::vector<AZ::u8>& pixelContent, QImage& image)
        {
            AZ::u32 bytesPerPixel = (AZ::u8)tgaHeader.m_bitsPerPixel / 8;
            
            image = QImage(tgaHeader.m_width, tgaHeader.m_height, QImage::Format_RGBA8888);

            for (unsigned int x = 0; x < tgaHeader.m_width; x++)
            {
                for (unsigned int y = 0; y < tgaHeader.m_height; y++)
                {
                    int r = pixelContent.at(y * tgaHeader.m_width * bytesPerPixel + x * bytesPerPixel + 2);
                    int g = pixelContent.at(y * tgaHeader.m_width * bytesPerPixel + x * bytesPerPixel + 1);
                    int b = pixelContent.at(y * tgaHeader.m_width * bytesPerPixel + x * bytesPerPixel);
                    int a = 255;
                    if (bytesPerPixel == 4)
                    {
                        a = pixelContent.at(y * tgaHeader.m_width * bytesPerPixel + x * bytesPerPixel + 3);
                    }

                    QColor value(r, g, b, a);
                    image.setPixelColor(x, y, value);
                }
            }

            image = image.mirrored();
        }

        bool IsExtensionSupported(const char* extension)
        {
            QList<QByteArray> imgFormats = QImageReader::supportedImageFormats();

            for (int i = 0; i < imgFormats.size(); ++i)
            {
                if (imgFormats[i].toLower().toStdString() == QString(extension).toLower().toStdString())
                {
                    return true;
                }
            }

            return false;
        }
    }//namespace QtImageLoader
} //namespace ImageProcessingAtom

