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
        static bool loadTga(const AZStd::string& filename, QImage& image);
        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
            
            //try to open the image
            QImage qimage(filename.c_str());
            if (qimage.isNull())
            {
                QString suffix = QFileInfo(filename.c_str()).suffix().toLower();
                if (suffix == "tga")
                {
                    if (!loadTga(filename, qimage))
                    {
                        AZ_Error("ImageProcessing", false, "Failed to load [%s] via QImage", filename.c_str());
                        return NULL;
                    }
                }
                else
                {
                    AZ_Error("ImageProcessing", false, "Failed to load [%s] via QImage", filename.c_str());
                    return NULL;
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
        static bool loadTga(const AZStd::string& filename, QImage& image)
        {
            if (!image.load(filename.c_str()))
            {
                // open the file
                FILE* pf = nullptr;
                azfopen(&pf, filename.c_str(), "rb");

                if (!pf)
                {
                    return false;
                }

                // some variables
                AZStd::vector<uint8_t>* vui8Pixels = nullptr;
                uint32_t ui32BpP;
                uint32_t ui32Width;
                uint32_t ui32Height;

                // read in the header
                uint8_t ui8x18Header[19] = { 0 };

                if (fread(reinterpret_cast<char*>(&ui8x18Header), sizeof(char), sizeof(ui8x18Header) - 1, pf) != sizeof(ui8x18Header) - 1)
                {
                    fclose(pf);
                    return false;
                }

                // get variables
                vui8Pixels = new AZStd::vector<uint8_t>;
                bool bCompressed;
                uint32_t ui32IDLength;
                uint32_t ui32PicType;
                uint32_t ui32PaletteLength;
                uint32_t ui32Size;

                // extract all information from header
                ui32IDLength = ui8x18Header[0];
                ui32PicType = ui8x18Header[2];
                ui32PaletteLength = ui8x18Header[6] * 0x100 + ui8x18Header[5];
                ui32Width = ui8x18Header[13] * 0x100 + ui8x18Header[12];
                ui32Height = ui8x18Header[15] * 0x100 + ui8x18Header[14];
                ui32BpP = ui8x18Header[16];

                // calculate some more information
                ui32Size = ui32Width * ui32Height * ui32BpP / 8;
                bCompressed = ui32PicType == 9 || ui32PicType == 10;
                vui8Pixels->resize(ui32Size);

                // jump to the data block
                fseek(pf, ui32IDLength + ui32PaletteLength, SEEK_CUR);

                if (ui32PicType == 2 && (ui32BpP == 24 || ui32BpP == 32))
                {
                    if (fread(reinterpret_cast<char*>(vui8Pixels->data()), sizeof(char), ui32Size, pf) != ui32Size)
                    {
                        delete vui8Pixels;
                        fclose(pf);
                        return false;
                    }
                }
                // else if compressed 24 or 32 bit
                else if (ui32PicType == 10 && (ui32BpP == 24 || ui32BpP == 32)) // compressed
                {
                    uint8_t tempChunkHeader;
                    uint8_t tempData[5];
                    unsigned int tempByteIndex = 0;

                    do
                    {
                        if (fread(reinterpret_cast<char*>(&tempChunkHeader), sizeof(char), sizeof(tempChunkHeader), pf) !=
                            sizeof(tempChunkHeader))
                        {
                            delete vui8Pixels;
                            fclose(pf);
                            return false;
                        }

                        if (tempChunkHeader >> 7) // repeat count
                        {
                            // just use the first 7 bits
                            tempChunkHeader = (uint8_t(tempChunkHeader << 1) >> 1);

                            if (fread(reinterpret_cast<char*>(&tempData), sizeof(char), ui32BpP / 8, pf) != ui32BpP / 8)
                            {
                                delete vui8Pixels;
                                fclose(pf);
                                return false;
                            }

                            for (int i = 0; i <= tempChunkHeader; i++)
                            {
                                vui8Pixels->at(tempByteIndex++) = tempData[0];
                                vui8Pixels->at(tempByteIndex++) = tempData[1];
                                vui8Pixels->at(tempByteIndex++) = tempData[2];
                                if (ui32BpP == 32)
                                {
                                    vui8Pixels->at(tempByteIndex++) = tempData[3];
                                }
                                    
                            }
                        }
                        else // data count
                        {
                            // just use the first 7 bits
                            tempChunkHeader = (uint8_t(tempChunkHeader << 1) >> 1);

                            for (int i = 0; i <= tempChunkHeader; i++)
                            {
                                if (fread(reinterpret_cast<char*>(&tempData), sizeof(char), ui32BpP / 8, pf) != ui32BpP / 8)
                                {
                                    delete vui8Pixels;
                                    fclose(pf);
                                    return false;
                                }
                                vui8Pixels->at(tempByteIndex++) = tempData[0];
                                vui8Pixels->at(tempByteIndex++) = tempData[1];
                                vui8Pixels->at(tempByteIndex++) = tempData[2];
                                if (ui32BpP == 32)
                                {
                                    vui8Pixels->at(tempByteIndex++) = tempData[3];
                                }
                                    
                            }
                        }
                    } while (tempByteIndex < ui32Size);
                }
                // not useable format
                else
                {
                    delete vui8Pixels;
                    fclose(pf);
                    return false;
                }
                                
                fclose(pf);

                image = QImage(ui32Width, ui32Height, QImage::Format_RGB888);

                int pixelSize = ui32BpP == 32 ? 4 : 3;
                // TODO: write direct into img
                for (unsigned int x = 0; x < ui32Width; x++)
                {
                    for (unsigned int y = 0; y < ui32Height; y++)
                    {
                        int valr = vui8Pixels->at(y * ui32Width * pixelSize + x * pixelSize + 2);
                        int valg = vui8Pixels->at(y * ui32Width * pixelSize + x * pixelSize + 1);
                        int valb = vui8Pixels->at(y * ui32Width * pixelSize + x * pixelSize);

                        QColor value(valr, valg, valb);
                        image.setPixelColor(x, y, value);
                    }
                }

                delete vui8Pixels;

                image = image.mirrored();
            }

            return true;
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

